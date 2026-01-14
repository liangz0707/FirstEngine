#pragma once

#include "FirstEngine/Device/Export.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Device context - provides basic Vulkan device-related information
        class FE_DEVICE_API DeviceContext {
        public:
            DeviceContext(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice, 
                         VkQueue graphicsQueue, VkQueue presentQueue,
                         VkCommandPool commandPool, uint32_t graphicsQueueFamily, 
                         uint32_t presentQueueFamily);
            ~DeviceContext();

            VkInstance GetInstance() const { return m_Instance; }
            VkDevice GetDevice() const { return m_Device; }
            VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
            VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
            VkQueue GetPresentQueue() const { return m_PresentQueue; }
            VkCommandPool GetCommandPool() const { return m_CommandPool; }
            uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
            uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }

        private:
            VkInstance m_Instance;
            VkDevice m_Device;
            VkPhysicalDevice m_PhysicalDevice;
            VkQueue m_GraphicsQueue;
            VkQueue m_PresentQueue;
            VkCommandPool m_CommandPool;
            uint32_t m_GraphicsQueueFamily;
            uint32_t m_PresentQueueFamily;
        };

    } // namespace Device
} // namespace FirstEngine
