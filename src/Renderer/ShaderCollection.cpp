#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <algorithm>

namespace FirstEngine {
    namespace Renderer {

        ShaderCollection::ShaderCollection() : m_Name(""), m_ID(0) {
        }

        ShaderCollection::ShaderCollection(const std::string& name, uint64_t id)
            : m_Name(name), m_ID(id) {
        }

        ShaderCollection::~ShaderCollection() = default;

        void ShaderCollection::AddShader(ShaderStage stage, std::unique_ptr<RHI::IShaderModule> shaderModule) {
            m_ShaderModules[stage] = std::move(shaderModule);
        }

        RHI::IShaderModule* ShaderCollection::GetShader(ShaderStage stage) const {
            auto it = m_ShaderModules.find(stage);
            if (it != m_ShaderModules.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        bool ShaderCollection::HasShader(ShaderStage stage) const {
            // Check SPIR-V code (shader modules are now managed by ShaderModuleTools)
            return m_SPIRVCode.find(stage) != m_SPIRVCode.end();
        }

        std::vector<ShaderStage> ShaderCollection::GetAvailableStages() const {
            std::vector<ShaderStage> stages;
            // Return stages based on SPIR-V code (not shader modules, which are now managed by ShaderModuleTools)
            stages.reserve(m_SPIRVCode.size());
            for (const auto& pair : m_SPIRVCode) {
                stages.push_back(pair.first);
            }
            return stages;
        }

        const std::vector<uint32_t>* ShaderCollection::GetSPIRVCode(ShaderStage stage) const {
            auto it = m_SPIRVCode.find(stage);
            if (it != m_SPIRVCode.end()) {
                return &it->second;
            }
            return nullptr;
        }

        void ShaderCollection::SetSPIRVCode(ShaderStage stage, const std::vector<uint32_t>& spirvCode) {
            m_SPIRVCode[stage] = spirvCode;
        }

        const std::string& ShaderCollection::GetMD5Hash(ShaderStage stage) const {
            static const std::string empty;
            auto it = m_MD5Hashes.find(stage);
            if (it != m_MD5Hashes.end()) {
                return it->second;
            }
            return empty;
        }

        void ShaderCollection::SetMD5Hash(ShaderStage stage, const std::string& hash) {
            m_MD5Hashes[stage] = hash;
        }

        void ShaderCollection::SetShaderReflection(std::unique_ptr<Shader::ShaderReflection> reflection) {
            m_ShaderReflection = std::move(reflection);
        }

        bool ShaderCollection::IsValid() const {
            // A valid collection should have at least vertex and fragment shaders with SPIR-V code
            return HasShader(ShaderStage::Vertex) && HasShader(ShaderStage::Fragment);
        }

    } // namespace Renderer
} // namespace FirstEngine
