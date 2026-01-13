#pragma once

#include "FirstEngine/Shader/Export.h"
#include <vector>
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Shader {
        enum class ShaderSourceLanguage {
            GLSL,
            HLSL
        };
        
        enum class ShaderStage {
            Vertex,
            Fragment,
            Geometry,
            TessellationControl,
            TessellationEvaluation,
            Compute
        };
        
        struct FE_SHADER_API CompileOptions {
            // Shader stage
            ShaderStage stage = ShaderStage::Vertex;
            
            // Source language
            ShaderSourceLanguage language = ShaderSourceLanguage::GLSL;
            
            // Entry point name
            std::string entry_point = "main";
            
            // Optimization level (0 = no optimization, 1-3 = optimization levels)
            int optimization_level = 0;
            
            // Generate debug info
            bool generate_debug_info = false;
            
            // Additional include directories
            std::vector<std::string> include_directories;
            
            // Defines/macros
            std::vector<std::pair<std::string, std::string>> defines;
            
            // Target environment (for HLSL)
            std::string target_profile; // e.g., "vs_6_0", "ps_6_0"
        };
        
        struct FE_SHADER_API CompileResult {
            bool success = false;
            std::vector<uint32_t> spirv_code; // Compiled SPIR-V bytecode
            std::string error_message;
            std::vector<std::string> warnings;
        };
        
        class FE_SHADER_API ShaderSourceCompiler {
        public:
            ShaderSourceCompiler();
            ~ShaderSourceCompiler();
            
            // Compile GLSL source code to SPIR-V
            CompileResult CompileGLSL(const std::string& source_code, const CompileOptions& options = CompileOptions());
            
            // Compile HLSL source code to SPIR-V
            CompileResult CompileHLSL(const std::string& source_code, const CompileOptions& options = CompileOptions());
            
            // Compile from file
            CompileResult CompileFromFile(const std::string& filepath, const CompileOptions& options = CompileOptions());
            
            // Auto-detect language from file extension and compile
            CompileResult CompileFromFileAuto(const std::string& filepath, const CompileOptions& options = CompileOptions());
            
            // Save compiled SPIR-V to file
            static bool SaveSPIRV(const std::vector<uint32_t>& spirv, const std::string& output_filepath);
            
        private:
            class Impl;
            std::unique_ptr<Impl> m_Impl;
        };
    }
}
