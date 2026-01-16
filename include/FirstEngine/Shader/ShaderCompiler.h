#pragma once

#include "FirstEngine/Shader/Export.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace FirstEngine {
    namespace Shader {
        enum class ShaderLanguage {
            GLSL,
            HLSL,
            MSL,
            SPIRV  // Source format
        };

        // Shader resource information (from AST)
        FE_SHADER_API struct ShaderResource {
            std::string name;
            uint32_t id;
            uint32_t type_id;
            uint32_t base_type_id;
            
            // Descriptor set information
            uint32_t set;
            uint32_t binding;
            
            // Buffer information
            uint32_t size;
            std::vector<uint32_t> array_size;
            
            // Type information (for vertex inputs)
            uint32_t location = 0; // Location decoration for vertex inputs
            uint32_t component = 0; // Component decoration
            uint32_t basetype = 0; // SPIRType basetype (0 = Unknown, 1 = Void, 2 = Boolean, 3 = Int, 4 = UInt, 5 = Float, etc.)
            uint32_t width = 0; // Type width (e.g., 32 for float32, 16 for float16)
            uint32_t vecsize = 0; // Vector size (1 = scalar, 2 = vec2, 3 = vec3, 4 = vec4)
            uint32_t columns = 0; // Matrix columns (0 = not a matrix, 2 = mat2, 3 = mat3, 4 = mat4)
        };

        // Uniform buffer information
        FE_SHADER_API struct UniformBuffer {
            std::string name;
            uint32_t id;
            uint32_t set;
            uint32_t binding;
            uint32_t size;
            std::vector<ShaderResource> members;
        };

        // Shader reflection data (AST-based)
        FE_SHADER_API struct ShaderReflection {
            ShaderLanguage language;
            std::vector<UniformBuffer> uniform_buffers;
            std::vector<ShaderResource> samplers;
            std::vector<ShaderResource> images;
            std::vector<ShaderResource> storage_buffers;
            std::vector<ShaderResource> stage_inputs;
            std::vector<ShaderResource> stage_outputs;
            std::string entry_point;
            uint32_t push_constant_size;
        };

        class FE_SHADER_API ShaderCompiler {
        public:
            // Construct from SPIR-V binary
            explicit ShaderCompiler(const std::vector<uint32_t>& spirv);
            
            // Construct from SPIR-V file
            explicit ShaderCompiler(const std::string& spirv_filepath);
            
            ~ShaderCompiler();

            // Convert to target language
            std::string CompileToGLSL(const std::string& entry_point = "main");
            std::string CompileToHLSL(const std::string& entry_point = "main");
            std::string CompileToMSL(const std::string& entry_point = "main");
            
            // Get reflection data (AST-based)
            ShaderReflection GetReflection() const;
            
            // Get all resources from AST
            std::vector<ShaderResource> GetUniformBuffers() const;
            std::vector<ShaderResource> GetSamplers() const;
            std::vector<ShaderResource> GetImages() const;
            std::vector<ShaderResource> GetStorageBuffers() const;
            
            // Access internal compiler (for advanced AST operations)
            // Note: Returns opaque pointer, cast to spirv_cross::Compiler* for use
            void* GetInternalCompiler() const { return m_Compiler; }
            
            // Set compiler options
            void SetGLSLVersion(uint32_t version);
            void SetHLSLShaderModel(uint32_t model); // e.g., 50 for Shader Model 5.0
            void SetMSLVersion(uint32_t version); // e.g., 20000 for MSL 2.0
            
        private:
            class Impl;
            std::unique_ptr<Impl> m_Impl;
            std::vector<uint32_t> m_SPIRV;
            void* m_Compiler = nullptr; // Opaque pointer to spirv_cross::Compiler
        };
    }
}
