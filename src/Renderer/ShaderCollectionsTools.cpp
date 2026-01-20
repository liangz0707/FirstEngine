#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderHash.h"
#include "FirstEngine/Shader/ShaderSourceCompiler.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
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

        ShaderCollectionsTools* ShaderCollectionsTools::s_Instance = nullptr;

        ShaderCollectionsTools& ShaderCollectionsTools::GetInstance() {
            if (!s_Instance) {
                s_Instance = new ShaderCollectionsTools();
            }
            return *s_Instance;
        }

        void ShaderCollectionsTools::Shutdown() {
            if (s_Instance) {
                s_Instance->Cleanup();
                delete s_Instance;
                s_Instance = nullptr;
            }
        }

        bool ShaderCollectionsTools::Initialize(const std::string& shaderDirectory) {
            if (m_Initialized) {
                return true; // Already initialized
            }

            m_ShaderDirectory = shaderDirectory;

            // Load all shaders from directory
            if (!LoadAllShadersFromDirectory(shaderDirectory)) {
                return false;
            }

            m_Initialized = true;
            return true;
        }

        void ShaderCollectionsTools::Cleanup() {
            m_Collections.clear();
            m_NameToID.clear();
            m_ShaderDirectory.clear();
            m_Initialized = false;
        }

        ShaderCollection* ShaderCollectionsTools::GetCollection(uint64_t id) const {
            auto it = m_Collections.find(id);
            if (it != m_Collections.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        ShaderCollection* ShaderCollectionsTools::GetCollectionByName(const std::string& name) const {
            auto it = m_NameToID.find(name);
            if (it != m_NameToID.end()) {
                return GetCollection(it->second);
            }
            return nullptr;
        }

        uint64_t ShaderCollectionsTools::AddCollection(std::unique_ptr<ShaderCollection> collection) {
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

        uint64_t ShaderCollectionsTools::CreateCollectionFromFiles(const std::string& shaderName, const std::string& shaderDirectory) {
            auto collection = std::make_unique<ShaderCollection>(shaderName, 0);

            std::vector<uint32_t> vertSPIRV;
            std::vector<uint32_t> fragSPIRV;

            // Try to find and compile vertex shader
            std::string vertPath = shaderDirectory + "/" + shaderName + ".vert.hlsl";
            if (fs::exists(vertPath)) {
                vertSPIRV = CompileHLSLToSPIRV(vertPath, ShaderStage::Vertex);
                if (!vertSPIRV.empty()) {
                    collection->SetSPIRVCode(ShaderStage::Vertex, vertSPIRV);
                    // Compute and store MD5 hash
                    std::string md5Hash = ShaderHash::ComputeMD5(vertSPIRV);
                    collection->SetMD5Hash(ShaderStage::Vertex, md5Hash);
                }
            }

            // Try to find and compile fragment shader
            std::string fragPath = shaderDirectory + "/" + shaderName + ".frag.hlsl";
            if (fs::exists(fragPath)) {
                fragSPIRV = CompileHLSLToSPIRV(fragPath, ShaderStage::Fragment);
                if (!fragSPIRV.empty()) {
                    collection->SetSPIRVCode(ShaderStage::Fragment, fragSPIRV);
                    // Compute and store MD5 hash
                    std::string md5Hash = ShaderHash::ComputeMD5(fragSPIRV);
                    collection->SetMD5Hash(ShaderStage::Fragment, md5Hash);
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

        bool ShaderCollectionsTools::LoadAllShadersFromDirectory(const std::string& shaderDirectory) {
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

        std::vector<uint64_t> ShaderCollectionsTools::GetAllCollectionIDs() const {
            std::vector<uint64_t> ids;
            ids.reserve(m_Collections.size());
            for (const auto& pair : m_Collections) {
                ids.push_back(pair.first);
            }
            return ids;
        }

        const Shader::ShaderReflection* ShaderCollectionsTools::GetShaderReflection(uint64_t collectionID) const {
            auto* collection = GetCollection(collectionID);
            if (collection) {
                return collection->GetShaderReflection();
            }
            return nullptr;
        }

        std::string ShaderCollectionsTools::GetShaderMD5(uint64_t collectionID, ShaderStage stage) const {
            auto* collection = GetCollection(collectionID);
            if (collection) {
                return collection->GetMD5Hash(stage);
            }
            return "";
        }

        ShaderStage ShaderCollectionsTools::DetectShaderStage(const std::string& filename) const {
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

        std::vector<uint32_t> ShaderCollectionsTools::CompileHLSLToSPIRV(const std::string& filepath, ShaderStage stage) const {
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

        std::string ShaderCollectionsTools::LoadShaderFile(const std::string& filepath) const {
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
