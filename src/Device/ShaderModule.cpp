#include "FirstEngine/Device/ShaderModule.h"
#include <fstream>
#include <stdexcept>

namespace FirstEngine {
    namespace Device {
        VkShaderModule ShaderModule::Create(VkDevice device, const std::vector<uint32_t>& spirv_code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = spirv_code.size() * sizeof(uint32_t);
            createInfo.pCode = spirv_code.data();

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shader module!");
            }

            return shaderModule;
        }
        
        VkShaderModule ShaderModule::CreateFromFile(VkDevice device, const std::string& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filepath);
            }
            
            size_t fileSize = (size_t)file.tellg();
            if (fileSize % 4 != 0) {
                throw std::runtime_error("SPIR-V file size must be a multiple of 4 bytes");
            }
            
            std::vector<uint32_t> buffer(fileSize / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            file.close();
            
            return Create(device, buffer);
        }
        
        void ShaderModule::Destroy(VkDevice device, VkShaderModule shaderModule) {
            if (shaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(device, shaderModule, nullptr);
            }
        }
    }
}
