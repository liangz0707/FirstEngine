#include "FirstEngine/Device/DeviceContext.h"

namespace FirstEngine {
    namespace Device {

        DeviceContext::DeviceContext(VkInstance instance, VkDevice device, VkPhysicalDevice physicalDevice,
                                   VkQueue graphicsQueue, VkQueue presentQueue,
                                   VkCommandPool commandPool, uint32_t graphicsQueueFamily,
                                   uint32_t presentQueueFamily)
            : m_Instance(instance), m_Device(device), m_PhysicalDevice(physicalDevice),
              m_GraphicsQueue(graphicsQueue), m_PresentQueue(presentQueue),
              m_CommandPool(commandPool), m_GraphicsQueueFamily(graphicsQueueFamily),
              m_PresentQueueFamily(presentQueueFamily) {
        }

        DeviceContext::~DeviceContext() {
            // DeviceContext不拥有这些资源，只是引用
        }

    } // namespace Device
} // namespace FirstEngine
