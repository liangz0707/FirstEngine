#include "FirstEngine/Shader/ShaderSourceCompiler.h"
#include "glslang/Public/ShaderLang.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Include/ResourceLimits.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Shader {
        
        class ShaderSourceCompiler::Impl {
        public:
            Impl() {
                // Initialize glslang
                glslang::InitializeProcess();
            }
            
            ~Impl() {
                // Finalize glslang
                glslang::FinalizeProcess();
            }
        };
        
        ShaderSourceCompiler::ShaderSourceCompiler()
            : m_Impl(std::make_unique<Impl>()) {
        }
        
        ShaderSourceCompiler::~ShaderSourceCompiler() = default;
        
        EShLanguage GetGlslangStage(ShaderStage stage) {
            switch (stage) {
                case ShaderStage::Vertex:
                    return EShLangVertex;
                case ShaderStage::Fragment:
                    return EShLangFragment;
                case ShaderStage::Geometry:
                    return EShLangGeometry;
                case ShaderStage::TessellationControl:
                    return EShLangTessControl;
                case ShaderStage::TessellationEvaluation:
                    return EShLangTessEvaluation;
                case ShaderStage::Compute:
                    return EShLangCompute;
                default:
                    return EShLangVertex;
            }
        }
        
        TBuiltInResource GetDefaultResources() {
            TBuiltInResource resources = {};
            resources.maxLights = 32;
            resources.maxClipPlanes = 6;
            resources.maxTextureUnits = 32;
            resources.maxTextureCoords = 32;
            resources.maxVertexAttribs = 64;
            resources.maxVertexUniformComponents = 4096;
            resources.maxVaryingFloats = 64;
            resources.maxVertexTextureImageUnits = 32;
            resources.maxCombinedTextureImageUnits = 80;
            resources.maxTextureImageUnits = 32;
            resources.maxFragmentUniformComponents = 4096;
            resources.maxDrawBuffers = 32;
            resources.maxVertexUniformVectors = 128;
            resources.maxVaryingVectors = 8;
            resources.maxFragmentUniformVectors = 16;
            resources.maxVertexOutputVectors = 16;
            resources.maxFragmentInputVectors = 15;
            resources.maxGeometryTextureImageUnits = 16;
            resources.maxGeometryUniformComponents = 1024;
            resources.maxGeometryVaryingComponents = 64;
            resources.maxTessControlInputComponents = 128;
            resources.maxTessControlOutputComponents = 128;
            resources.maxTessControlTextureImageUnits = 16;
            resources.maxTessControlUniformComponents = 1024;
            resources.maxTessControlTotalOutputComponents = 4096;
            resources.maxTessEvaluationInputComponents = 128;
            resources.maxTessEvaluationOutputComponents = 128;
            resources.maxTessEvaluationTextureImageUnits = 16;
            resources.maxTessEvaluationUniformComponents = 1024;
            resources.maxTessPatchComponents = 120;
            resources.maxPatchVertices = 32;
            resources.maxTessGenLevel = 64;
            resources.maxViewports = 16;
            resources.maxVertexAtomicCounters = 0;
            resources.maxTessControlAtomicCounters = 0;
            resources.maxTessEvaluationAtomicCounters = 0;
            resources.maxGeometryAtomicCounters = 0;
            resources.maxFragmentAtomicCounters = 0;
            resources.maxCombinedAtomicCounters = 0;
            resources.maxAtomicCounterBindings = 0;
            resources.maxVertexAtomicCounterBuffers = 0;
            resources.maxTessControlAtomicCounterBuffers = 0;
            resources.maxTessEvaluationAtomicCounterBuffers = 0;
            resources.maxGeometryAtomicCounterBuffers = 0;
            resources.maxFragmentAtomicCounterBuffers = 0;
            resources.maxCombinedAtomicCounterBuffers = 0;
            resources.maxAtomicCounterBufferSize = 0;
            resources.maxTransformFeedbackBuffers = 4;
            resources.maxTransformFeedbackInterleavedComponents = 64;
            resources.maxCullDistances = 8;
            resources.maxCombinedClipAndCullDistances = 8;
            resources.maxSamples = 4;
            resources.limits.nonInductiveForLoops = 1;
            resources.limits.whileLoops = 1;
            resources.limits.doWhileLoops = 1;
            resources.limits.generalUniformIndexing = 1;
            resources.limits.generalAttributeMatrixVectorIndexing = 1;
            resources.limits.generalVaryingIndexing = 1;
            resources.limits.generalSamplerIndexing = 1;
            resources.limits.generalVariableIndexing = 1;
            resources.limits.generalConstantMatrixVectorIndexing = 1;
            return resources;
        }
        
        CompileResult CompileGLSLToSPIRV(const std::string& source_code, EShLanguage stage, 
                                         const CompileOptions& options, TBuiltInResource& resources) {
            CompileResult result;
            
            // Create shader object
            glslang::TShader shader(stage);
            const char* shader_strings = source_code.c_str();
            
            shader.setStrings(&shader_strings, 1);
            shader.setEntryPoint(options.entry_point.c_str());
            shader.setSourceEntryPoint(options.entry_point.c_str());
            
            // Set defines/macros
            for (const auto& define : options.defines) {
                std::string def_str = define.first;
                if (!define.second.empty()) {
                    def_str += "=" + define.second;
                }
                shader.setPreamble((def_str + "\n").c_str());
            }
            
            // Parse shader
            EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
            
            if (!shader.parse(&resources, 100, false, messages)) {
                result.success = false;
                result.error_message = shader.getInfoLog();
                result.error_message += "\n";
                result.error_message += shader.getInfoDebugLog();
                return result;
            }
            
            // Get warnings
            if (shader.getInfoLog() && strlen(shader.getInfoLog()) > 0) {
                result.warnings.push_back(shader.getInfoLog());
            }
            
            // Link (for multi-stage shaders, but we're doing single-stage)
            glslang::TProgram program;
            program.addShader(&shader);
            
            if (!program.link(messages)) {
                result.success = false;
                result.error_message = program.getInfoLog();
                result.error_message += "\n";
                result.error_message = program.getInfoDebugLog();
                return result;
            }
            
            // Convert to SPIR-V
            std::vector<uint32_t> spirv;
            spv::SpvBuildLogger logger;
            glslang::SpvOptions spvOptions;
            spvOptions.generateDebugInfo = options.generate_debug_info;
            spvOptions.disableOptimizer = (options.optimization_level == 0);
            spvOptions.optimizeSize = (options.optimization_level == 2);
            
            glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &spvOptions);
            
            if (logger.getAllMessages().length() > 0) {
                result.warnings.push_back(logger.getAllMessages());
            }
            
            result.success = true;
            result.spirv_code = spirv;
            
            return result;
        }
        
        CompileResult ShaderSourceCompiler::CompileGLSL(const std::string& source_code, const CompileOptions& options) {
            EShLanguage stage = GetGlslangStage(options.stage);
            TBuiltInResource resources = GetDefaultResources();
            
            return CompileGLSLToSPIRV(source_code, stage, options, resources);
        }
        
        CompileResult ShaderSourceCompiler::CompileHLSL(const std::string& source_code, const CompileOptions& options) {
            CompileResult result;
            
            // Note: glslang has limited HLSL support
            // For full HLSL support, you may need DXC (DirectX Shader Compiler)
            // This implementation uses glslang's HLSL support which may have limitations
            
            EShLanguage stage = GetGlslangStage(options.stage);
            TBuiltInResource resources = GetDefaultResources();
            
            glslang::TShader shader(stage);
            const char* shader_strings = source_code.c_str();
            
            shader.setStrings(&shader_strings, 1);
            shader.setEntryPoint(options.entry_point.c_str());
            shader.setSourceEntryPoint(options.entry_point.c_str());
            
            // Set HLSL mode
            shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientVulkan, 100);
            shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
            shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);
            
            // Set defines
            for (const auto& define : options.defines) {
                std::string def_str = define.first;
                if (!define.second.empty()) {
                    def_str += "=" + define.second;
                }
                shader.setPreamble((def_str + "\n").c_str());
            }
            
            EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
            
            if (!shader.parse(&resources, 100, false, messages)) {
                result.success = false;
                result.error_message = shader.getInfoLog();
                result.error_message += "\n";
                result.error_message += shader.getInfoDebugLog();
                return result;
            }
            
            if (shader.getInfoLog() && strlen(shader.getInfoLog()) > 0) {
                result.warnings.push_back(shader.getInfoLog());
            }
            
            glslang::TProgram program;
            program.addShader(&shader);
            
            if (!program.link(messages)) {
                result.success = false;
                result.error_message = program.getInfoLog();
                result.error_message += "\n";
                result.error_message = program.getInfoDebugLog();
                return result;
            }
            
            std::vector<uint32_t> spirv;
            spv::SpvBuildLogger logger;
            glslang::SpvOptions spvOptions;
            spvOptions.generateDebugInfo = options.generate_debug_info;
            spvOptions.disableOptimizer = (options.optimization_level == 0);
            spvOptions.optimizeSize = (options.optimization_level == 2);
            
            glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &spvOptions);
            
            if (logger.getAllMessages().length() > 0) {
                result.warnings.push_back(logger.getAllMessages());
            }
            
            result.success = true;
            result.spirv_code = spirv;
            
            return result;
        }
        
        CompileResult ShaderSourceCompiler::CompileFromFile(const std::string& filepath, const CompileOptions& options) {
            CompileResult result;
            
            try {
                std::ifstream file(filepath);
                if (!file.is_open()) {
                    result.success = false;
                    result.error_message = "Failed to open file: " + filepath;
                    return result;
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string source_code = buffer.str();
                file.close();
                
                // Determine language from options
                if (options.language == ShaderSourceLanguage::GLSL) {
                    return CompileGLSL(source_code, options);
                } else {
                    return CompileHLSL(source_code, options);
                }
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = std::string("Exception: ") + e.what();
                return result;
            }
        }
        
        CompileResult ShaderSourceCompiler::CompileFromFileAuto(const std::string& filepath, const CompileOptions& options) {
            CompileResult result;
            
            try {
                // Auto-detect language from file extension
                std::filesystem::path path(filepath);
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                CompileOptions opts = options;
                
                // Detect shader stage from extension
                if (ext == ".vert" || ext == ".vertex") {
                    opts.stage = ShaderStage::Vertex;
                } else if (ext == ".frag" || ext == ".fragment") {
                    opts.stage = ShaderStage::Fragment;
                } else if (ext == ".geom" || ext == ".geometry") {
                    opts.stage = ShaderStage::Geometry;
                } else if (ext == ".comp" || ext == ".compute") {
                    opts.stage = ShaderStage::Compute;
                } else if (ext == ".tesc" || ext == ".tesscontrol") {
                    opts.stage = ShaderStage::TessellationControl;
                } else if (ext == ".tese" || ext == ".tesseval") {
                    opts.stage = ShaderStage::TessellationEvaluation;
                }
                
                // Detect language from extension
                if (ext == ".hlsl" || ext == ".fx" || ext == ".fxh") {
                    opts.language = ShaderSourceLanguage::HLSL;
                } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || 
                          ext == ".geom" || ext == ".comp" || ext == ".tesc" || ext == ".tese" ||
                          ext == ".vs" || ext == ".fs" || ext == ".gs" || ext == ".cs") {
                    opts.language = ShaderSourceLanguage::GLSL;
                }
                
                return CompileFromFile(filepath, opts);
                
            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = std::string("Exception: ") + e.what();
                return result;
            }
        }
        
        bool ShaderSourceCompiler::SaveSPIRV(const std::vector<uint32_t>& spirv, const std::string& output_filepath) {
            try {
                std::ofstream file(output_filepath, std::ios::binary);
                if (!file.is_open()) {
                    return false;
                }
                
                file.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
                file.close();
                
                return true;
            } catch (...) {
                return false;
            }
        }
        
    }
}
