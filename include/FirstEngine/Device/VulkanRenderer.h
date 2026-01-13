#pragma once

#include "FirstEngine/Device/Export.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include "FirstEngine/Core/Window.h"

namespace FirstEngine {
    namespace Device {
        class Swapchain;
        class Pipeline;

        class FE_DEVICE_API VulkanRenderer {
        public:
            VulkanRenderer(Core::Window* window);
            ~VulkanRenderer();

            void OnWindowResize(int width, int height);
            void Present();
            void WaitIdle();

            VkDevice GetDevice() const { return m_Device; }
            VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
            VkInstance GetInstance() const { return m_Instance; }
            VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
            VkQueue GetPresentQueue() const { return m_PresentQueue; }
            VkCommandPool GetCommandPool() const { return m_CommandPool; }
            Core::Window* GetWindow() const { return m_Window; }
            VkSurfaceKHR GetSurface() const { return m_Surface; }
            uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
            uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }
            Swapchain* GetSwapchain() const { return m_Swapchain.get(); }

        private:
            void CreateInstance();
            void SetupDebugMessenger();
            void CreateSurface();
            void PickPhysicalDevice();
            void CreateLogicalDevice();
            void CreateCommandPool();
            void CreateSyncObjects();

            bool CheckValidationLayerSupport();
            std::vector<const char*> GetRequiredExtensions();

            Core::Window* m_Window;
            VkInstance m_Instance;
            VkDebugUtilsMessengerEXT m_DebugMessenger;
            VkPhysicalDevice m_PhysicalDevice;
            VkDevice m_Device;
            VkQueue m_GraphicsQueue;
            VkQueue m_PresentQueue;
            VkCommandPool m_CommandPool;
            uint32_t m_GraphicsQueueFamily;
            uint32_t m_PresentQueueFamily;
            VkSurfaceKHR m_Surface;

            std::unique_ptr<Swapchain> m_Swapchain;
            std::unique_ptr<Pipeline> m_Pipeline;

            VkSemaphore m_ImageAvailableSemaphore;
            VkSemaphore m_RenderFinishedSemaphore;
            VkFence m_InFlightFence;

            const std::vector<const char*> m_ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"
            };

            const std::vector<const char*> m_DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };

            bool m_EnableValidationLayers = true;

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);
        };
    }
}
