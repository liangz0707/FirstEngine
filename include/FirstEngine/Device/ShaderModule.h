#pragma once

#include "FirstEngine/Device/Export.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace FirstEngine {
    namespace Device {
        // Shader module creation (Vulkan API specific)
        class FE_DEVICE_API ShaderModule {
        public:
            // Create shader module from SPIR-V bytecode
            static VkShaderModule Create(VkDevice device, const std::vector<uint32_t>& spirv_code);
            
            // Create shader module from SPIR-V file
            static VkShaderModule CreateFromFile(VkDevice device, const std::string& filepath);
            
            // Destroy shader module
            static void Destroy(VkDevice device, VkShaderModule shaderModule);
        };
    }
}
