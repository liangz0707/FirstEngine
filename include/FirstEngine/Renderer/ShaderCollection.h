#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/Shader/ShaderCompiler.h"  // For ShaderReflection complete definition
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // Shader stage enumeration for accessing shaders in collection
        enum class ShaderStage {
            Vertex,
            Fragment,
            Geometry,
            TessellationControl,
            TessellationEvaluation,
            Compute
        };

        // ShaderCollection - stores a collection of shader modules for different stages
        // Typically contains vertex and fragment shaders as a pair
        class FE_RENDERER_API ShaderCollection {
        public:
            ShaderCollection();
            ShaderCollection(const std::string& name, uint64_t id);
            inline ~ShaderCollection() {};

            // Get collection name
            const std::string& GetName() const { return m_Name; }
            void SetName(const std::string& name) { m_Name = name; }

            // Get collection ID
            uint64_t GetID() const { return m_ID; }
            void SetID(uint64_t id) { m_ID = id; }

            // NOTE: Shader modules are now managed by ShaderModuleTools, not stored in ShaderCollection
            // These methods are kept for backward compatibility but may be deprecated in a future version
            // TODO: Consider deprecating these methods - use ShaderModuleTools instead
            // Add shader module for a specific stage (deprecated - use ShaderModuleTools instead)
            void AddShader(ShaderStage stage, std::unique_ptr<RHI::IShaderModule> shaderModule);

            // Get shader module for a specific stage (deprecated - use ShaderModuleTools instead)
            RHI::IShaderModule* GetShader(ShaderStage stage) const;

            // Check if shader exists for a stage (checks SPIR-V code, not shader modules)
            bool HasShader(ShaderStage stage) const;

            // Get all stages that have shaders
            std::vector<ShaderStage> GetAvailableStages() const;

            // Get SPIR-V code for a specific stage (for creating shader modules)
            const std::vector<uint32_t>* GetSPIRVCode(ShaderStage stage) const;
            void SetSPIRVCode(ShaderStage stage, const std::vector<uint32_t>& spirvCode);

            // Get MD5 hash for a specific stage (computed from SPIR-V code)
            const std::string& GetMD5Hash(ShaderStage stage) const;
            void SetMD5Hash(ShaderStage stage, const std::string& hash);

            // Shader reflection (parsed during shader loading)
            const Shader::ShaderReflection* GetShaderReflection() const { return m_ShaderReflection.get(); }
            void SetShaderReflection(std::unique_ptr<Shader::ShaderReflection> reflection);

            // Check if collection is valid (has at least vertex and fragment shaders with SPIR-V code)
            bool IsValid() const;

        private:
            std::string m_Name;
            uint64_t m_ID;

            // Shader modules by stage
            std::unordered_map<ShaderStage, std::unique_ptr<RHI::IShaderModule>> m_ShaderModules;

            // SPIR-V code by stage (stored separately for lazy module creation)
            std::unordered_map<ShaderStage, std::vector<uint32_t>> m_SPIRVCode;

            // MD5 hash by stage (computed from SPIR-V code for cache lookup)
            std::unordered_map<ShaderStage, std::string> m_MD5Hashes;

            // Shader reflection data (parsed once during loading, cached for reuse)
            std::unique_ptr<Shader::ShaderReflection> m_ShaderReflection;
        };

    } // namespace Renderer
} // namespace FirstEngine
