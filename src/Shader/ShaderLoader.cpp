#include "FirstEngine/Shader/ShaderLoader.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace FirstEngine {
    namespace Shader {
        std::vector<char> ShaderLoader::LoadShader(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filepath);
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }

        VkShaderModule ShaderLoader::CreateShaderModule(VkDevice device, const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shader module!");
            }

            return shaderModule;
        }
    }
}
