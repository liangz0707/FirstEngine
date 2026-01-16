#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Shader/ShaderSourceCompiler.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Renderer {

        ShaderModuleTools* ShaderModuleTools::s_Instance = nullptr;

        ShaderModuleTools& ShaderModuleTools::GetInstance() {
            if (!s_Instance) {
                s_Instance = new ShaderModuleTools();
            }
            return *s_Instance;
        }

        void ShaderModuleTools::Shutdown() {
            if (s_Instance) {
                s_Instance->Cleanup();
                delete s_Instance;
                s_Instance = nullptr;
            }
        }

        bool ShaderModuleTools::Initialize(const std::string& shaderDirectory, RHI::IDevice* device) {
            if (m_Initialized) {
                return true; // Already initialized
            }

            m_ShaderDirectory = shaderDirectory;
            m_Device = device;

            // Load all shaders from directory
            if (!LoadAllShadersFromDirectory(shaderDirectory)) {
                return false;
            }

            m_Initialized = true;
            return true;
        }

        void ShaderModuleTools::Cleanup() {
            m_Collections.clear();
            m_NameToID.clear();
            m_Device = nullptr;
            m_ShaderDirectory.clear();
            m_Initialized = false;
        }

        ShaderCollection* ShaderModuleTools::GetCollection(uint64_t id) const {
            auto it = m_Collections.find(id);
            if (it != m_Collections.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        ShaderCollection* ShaderModuleTools::GetCollectionByName(const std::string& name) const {
            auto it = m_NameToID.find(name);
            if (it != m_NameToID.end()) {
                return GetCollection(it->second);
            }
            return nullptr;
        }

        uint64_t ShaderModuleTools::AddCollection(std::unique_ptr<ShaderCollection> collection) {
            if (!collection) {
                return 0;
            }

            uint64_t id = m_NextID++;
            collection->SetID(id);
            
            std::string name = collection->GetName();
            if (!name.empty()) {
                m_NameToID[name] = id;
            }

            m_Collections[id] = std::move(collection);
            return id;
        }

        uint64_t ShaderModuleTools::CreateCollectionFromFiles(const std::string& shaderName, const std::string& shaderDirectory) {
            auto collection = std::make_unique<ShaderCollection>(shaderName, 0);

            std::vector<uint32_t> vertSPIRV;
            std::vector<uint32_t> fragSPIRV;

            // Try to find and compile vertex shader
            std::string vertPath = shaderDirectory + "/" + shaderName + ".vert.hlsl";
            if (fs::exists(vertPath)) {
                vertSPIRV = CompileHLSLToSPIRV(vertPath, ShaderStage::Vertex);
                if (!vertSPIRV.empty()) {
                    collection->SetSPIRVCode(ShaderStage::Vertex, vertSPIRV);
                    
                    // Create shader module if device is available
                    if (m_Device) {
                        auto shaderModule = m_Device->CreateShaderModule(vertSPIRV, RHI::ShaderStage::Vertex);
                        if (shaderModule) {
                            collection->AddShader(ShaderStage::Vertex, std::move(shaderModule));
                        }
                    }
                }
            }

            // Try to find and compile fragment shader
            std::string fragPath = shaderDirectory + "/" + shaderName + ".frag.hlsl";
            if (fs::exists(fragPath)) {
                fragSPIRV = CompileHLSLToSPIRV(fragPath, ShaderStage::Fragment);
                if (!fragSPIRV.empty()) {
                    collection->SetSPIRVCode(ShaderStage::Fragment, fragSPIRV);
                    
                    // Create shader module if device is available
                    if (m_Device) {
                        auto shaderModule = m_Device->CreateShaderModule(fragSPIRV, RHI::ShaderStage::Fragment);
                        if (shaderModule) {
                            collection->AddShader(ShaderStage::Fragment, std::move(shaderModule));
                        }
                    }
                }
            }

            // Only add if collection has at least one shader
            if (collection->GetAvailableStages().empty()) {
                return 0;
            }

            // Parse shader reflection (prefer vertex shader, fallback to fragment)
            std::vector<uint32_t> reflectionCode = vertSPIRV.empty() ? fragSPIRV : vertSPIRV;
            if (!reflectionCode.empty()) {
                try {
                    Shader::ShaderCompiler compiler(reflectionCode);
                    auto reflection = compiler.GetReflection();
                    // Store reflection in collection
                    auto reflectionPtr = std::make_unique<Shader::ShaderReflection>(std::move(reflection));
                    collection->SetShaderReflection(std::move(reflectionPtr));
                } catch (...) {
                    // Failed to parse reflection, but collection is still valid
                    // Reflection will be parsed later if needed
                }
            }

            return AddCollection(std::move(collection));
        }

        bool ShaderModuleTools::LoadAllShadersFromDirectory(const std::string& shaderDirectory) {
            if (!fs::exists(shaderDirectory) || !fs::is_directory(shaderDirectory)) {
                return false;
            }

            // Map to store shader names found
            std::unordered_map<std::string, std::vector<std::string>> shaderFiles;

            // Scan directory for HLSL files
            for (const auto& entry : fs::directory_iterator(shaderDirectory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    std::string extension = entry.path().extension().string();
                    
                    if (extension == ".hlsl") {
                        // Parse filename: {Name}.{stage}.hlsl
                        size_t firstDot = filename.find('.');
                        if (firstDot != std::string::npos) {
                            std::string baseName = filename.substr(0, firstDot);
                            size_t secondDot = filename.find('.', firstDot + 1);
                            if (secondDot != std::string::npos) {
                                std::string stageStr = filename.substr(firstDot + 1, secondDot - firstDot - 1);
                                shaderFiles[baseName].push_back(entry.path().string());
                            }
                        }
                    }
                }
            }

            // Create collections for each shader name
            for (const auto& pair : shaderFiles) {
                const std::string& shaderName = pair.first;
                CreateCollectionFromFiles(shaderName, shaderDirectory);
            }

            return true;
        }

        std::vector<uint64_t> ShaderModuleTools::GetAllCollectionIDs() const {
            std::vector<uint64_t> ids;
            ids.reserve(m_Collections.size());
            for (const auto& pair : m_Collections) {
                ids.push_back(pair.first);
            }
            return ids;
        }

        void ShaderModuleTools::SetDevice(RHI::IDevice* device) {
            m_Device = device;
            
            // Create shader modules for all collections that have SPIR-V but no modules
            for (auto& pair : m_Collections) {
                auto& collection = pair.second;
                
                // Check each stage
                for (int stageInt = static_cast<int>(ShaderStage::Vertex); 
                     stageInt <= static_cast<int>(ShaderStage::Compute); 
                     stageInt++) {
                    ShaderStage stage = static_cast<ShaderStage>(stageInt);
                    
                    // If has SPIR-V but no shader module, create it
                    if (collection->GetSPIRVCode(stage) && !collection->HasShader(stage)) {
                        const std::vector<uint32_t>* spirvCode = collection->GetSPIRVCode(stage);
                        if (spirvCode && !spirvCode->empty() && m_Device) {
                            // Map Renderer::ShaderStage to RHI::ShaderStage
                            RHI::ShaderStage rhiStage = RHI::ShaderStage::Vertex;
                            switch (stage) {
                                case ShaderStage::Vertex:
                                    rhiStage = RHI::ShaderStage::Vertex;
                                    break;
                                case ShaderStage::Fragment:
                                    rhiStage = RHI::ShaderStage::Fragment;
                                    break;
                                case ShaderStage::Geometry:
                                    rhiStage = RHI::ShaderStage::Geometry;
                                    break;
                                case ShaderStage::Compute:
                                    rhiStage = RHI::ShaderStage::Compute;
                                    break;
                                case ShaderStage::TessellationControl:
                                    rhiStage = RHI::ShaderStage::TessellationControl;
                                    break;
                                case ShaderStage::TessellationEvaluation:
                                    rhiStage = RHI::ShaderStage::TessellationEvaluation;
                                    break;
                            }
                            
                            auto shaderModule = m_Device->CreateShaderModule(*spirvCode, rhiStage);
                            if (shaderModule) {
                                collection->AddShader(stage, std::move(shaderModule));
                            }
                        }
                    }
                }
            }
        }

        ShaderStage ShaderModuleTools::DetectShaderStage(const std::string& filename) const {
            std::string lowerFilename = filename;
            std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

            if (lowerFilename.find(".vert.") != std::string::npos) {
                return ShaderStage::Vertex;
            } else if (lowerFilename.find(".frag.") != std::string::npos) {
                return ShaderStage::Fragment;
            } else if (lowerFilename.find(".geom.") != std::string::npos) {
                return ShaderStage::Geometry;
            } else if (lowerFilename.find(".comp.") != std::string::npos) {
                return ShaderStage::Compute;
            } else if (lowerFilename.find(".tesc.") != std::string::npos) {
                return ShaderStage::TessellationControl;
            } else if (lowerFilename.find(".tese.") != std::string::npos) {
                return ShaderStage::TessellationEvaluation;
            }

            return ShaderStage::Vertex; // Default
        }

        std::vector<uint32_t> ShaderModuleTools::CompileHLSLToSPIRV(const std::string& filepath, ShaderStage stage) const {
            FirstEngine::Shader::ShaderSourceCompiler compiler;
            
            // Set compile options
            FirstEngine::Shader::CompileOptions options;
            options.language = FirstEngine::Shader::ShaderSourceLanguage::HLSL;
            
            // Map Renderer::ShaderStage to Shader::ShaderStage
            switch (stage) {
                case ShaderStage::Vertex:
                    options.stage = FirstEngine::Shader::ShaderStage::Vertex;
                    options.target_profile = "vs_6_0";
                    break;
                case ShaderStage::Fragment:
                    options.stage = FirstEngine::Shader::ShaderStage::Fragment;
                    options.target_profile = "ps_6_0";
                    break;
                case ShaderStage::Geometry:
                    options.stage = FirstEngine::Shader::ShaderStage::Geometry;
                    options.target_profile = "gs_6_0";
                    break;
                case ShaderStage::Compute:
                    options.stage = FirstEngine::Shader::ShaderStage::Compute;
                    options.target_profile = "cs_6_0";
                    break;
                case ShaderStage::TessellationControl:
                    options.stage = FirstEngine::Shader::ShaderStage::TessellationControl;
                    options.target_profile = "hs_6_0";
                    break;
                case ShaderStage::TessellationEvaluation:
                    options.stage = FirstEngine::Shader::ShaderStage::TessellationEvaluation;
                    options.target_profile = "ds_6_0";
                    break;
            }
            
            options.entry_point = "main";
            options.optimization_level = 1;

            // Compile from file
            auto result = compiler.CompileFromFileAuto(filepath, options);
            
            if (result.success) {
                return result.spirv_code;
            } else {
                std::cerr << "Failed to compile shader: " << filepath << std::endl;
                std::cerr << "Error: " << result.error_message << std::endl;
                return std::vector<uint32_t>();
            }
        }

        std::string ShaderModuleTools::LoadShaderFile(const std::string& filepath) const {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                return "";
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();

            return buffer.str();
        }

    } // namespace Renderer
} // namespace FirstEngine
