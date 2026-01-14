#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Shader module wrapper (object-oriented design)
        class FE_DEVICE_API ShaderModule {
        public:
            ShaderModule(DeviceContext* context);
            ~ShaderModule();

            // Create shader module from SPIR-V bytecode
            bool Create(const std::vector<uint32_t>& spirvCode);

            // Create shader module from SPIR-V file
            bool CreateFromFile(const std::string& filepath);

            // Destroy shader module
            void Destroy();

            VkShaderModule GetShaderModule() const { return m_ShaderModule; }
            VkShaderStageFlagBits GetStage() const { return m_Stage; }
            bool IsValid() const { return m_ShaderModule != VK_NULL_HANDLE; }

        private:
            DeviceContext* m_Context;
            VkShaderModule m_ShaderModule;
            VkShaderStageFlagBits m_Stage;
        };

    } // namespace Device
} // namespace FirstEngine
