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
            return m_ShaderModules.find(stage) != m_ShaderModules.end();
        }

        std::vector<ShaderStage> ShaderCollection::GetAvailableStages() const {
            std::vector<ShaderStage> stages;
            stages.reserve(m_ShaderModules.size());
            for (const auto& pair : m_ShaderModules) {
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

        void ShaderCollection::SetShaderReflection(std::unique_ptr<Shader::ShaderReflection> reflection) {
            m_ShaderReflection = std::move(reflection);
        }

        bool ShaderCollection::IsValid() const {
            // A valid collection should have at least vertex and fragment shaders
            return HasShader(ShaderStage::Vertex) && HasShader(ShaderStage::Fragment);
        }

    } // namespace Renderer
} // namespace FirstEngine
