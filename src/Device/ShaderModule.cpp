#include "FirstEngine/Device/ShaderModule.h"
#include "FirstEngine/Device/DeviceContext.h"
#include "FirstEngine/Shader/ShaderLoader.h"
#include <fstream>
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        ShaderModule::ShaderModule(DeviceContext* context)
            : m_Context(context), m_ShaderModule(VK_NULL_HANDLE),
              m_Stage(VK_SHADER_STAGE_VERTEX_BIT) {
        }

        ShaderModule::~ShaderModule() {
            Destroy();
        }

        bool ShaderModule::Create(const std::vector<uint32_t>& spirvCode) {
            if (spirvCode.empty()) {
                return false;
            }

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
            createInfo.pCode = spirvCode.data();

            if (vkCreateShaderModule(m_Context->GetDevice(), &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        bool ShaderModule::CreateFromFile(const std::string& filepath) {
            
            auto spirvCode = FirstEngine::Shader::ShaderLoader::LoadSPIRV(filepath);
            if (spirvCode.empty()) {
                return false;
            }

            return Create(spirvCode);
        }

        void ShaderModule::Destroy() {
            if (m_ShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(m_Context->GetDevice(), m_ShaderModule, nullptr);
                m_ShaderModule = VK_NULL_HANDLE;
            }
        }

    } // namespace Device
} // namespace FirstEngine
