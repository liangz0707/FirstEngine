#pragma once

#include "FirstEngine/Shader/ShaderSourceCompiler.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <string>
#include <vector>
#include <map>

namespace FirstEngine {
    namespace ShaderManager {
        
        enum class Command {
            Compile,
            Convert,
            Reflect,
            Help,
            Unknown
        };
        
        enum class OutputFormat {
            SPIRV,
            GLSL,
            HLSL,
            MSL
        };
        
        struct CompileOptions {
            std::string input_file;
            std::string output_file;
            FirstEngine::Shader::ShaderStage stage = FirstEngine::Shader::ShaderStage::Vertex;
            FirstEngine::Shader::ShaderSourceLanguage language = FirstEngine::Shader::ShaderSourceLanguage::GLSL;
            std::string entry_point = "main";
            int optimization_level = 0;
            bool generate_debug_info = false;
            std::vector<std::pair<std::string, std::string>> defines;
        };
        
        struct ConvertOptions {
            std::string input_file;  // SPIR-V file
            std::string output_file;
            OutputFormat target_format = OutputFormat::GLSL;
            std::string entry_point = "main";
        };
        
        struct ReflectOptions {
            std::string input_file;  // SPIR-V file
            bool show_resources = true;
            bool show_uniform_buffers = true;
            bool show_samplers = true;
            bool show_images = true;
            bool show_storage_buffers = true;
        };
        
        class ShaderManager {
        public:
            ShaderManager();
            ~ShaderManager();
            
            // Parse command line arguments
            bool ParseArguments(int argc, char* argv[]);
            
            // Execute the parsed command
            int Execute();
            
            // Print help message
            void PrintHelp() const;
            
        private:
            Command m_Command = Command::Unknown;
            CompileOptions m_CompileOptions;
            ConvertOptions m_ConvertOptions;
            ReflectOptions m_ReflectOptions;
            
            // Command execution
            int ExecuteCompile();
            int ExecuteConvert();
            int ExecuteReflect();
            
            // Helper functions
            FirstEngine::Shader::ShaderStage ParseStage(const std::string& stage_str);
            FirstEngine::Shader::ShaderSourceLanguage ParseLanguage(const std::string& lang_str);
            OutputFormat ParseOutputFormat(const std::string& format_str);
            std::string GetFileExtension(const std::string& filepath);
            void AutoDetectOptions(CompileOptions& options);
        };
        
    }
}
