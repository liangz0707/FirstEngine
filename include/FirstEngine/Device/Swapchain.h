#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "FirstEngine/Core/Window.h"

namespace FirstEngine {
    namespace Device {

        // Swapchain wrapper (object-oriented design)
        class FE_DEVICE_API Swapchain {
        public:
            Swapchain(DeviceContext* context, Core::Window* window, VkSurfaceKHR surface);
            ~Swapchain();

            // Create swapchain
            bool Create();

            // Recreate swapchain (when window size changes)
            bool Recreate();

            // Submit presentation
            bool Present(uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);

            // Get next frame image index
            bool AcquireNextImage(VkSemaphore semaphore, VkFence fence, uint32_t& imageIndex);

            // Destroy swapchain
            void Destroy();

            VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
            VkFormat GetImageFormat() const { return m_ImageFormat; }
            VkExtent2D GetExtent() const { return m_Extent; }
            const std::vector<VkImage>& GetImages() const { return m_Images; }
            const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
            uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
            bool IsValid() const { return m_Swapchain != VK_NULL_HANDLE; }

        private:
            void CreateSwapchain();
            void CreateImageViews();
            void CleanupSwapchain();

            VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

            DeviceContext* m_Context;
            Core::Window* m_Window;
            VkSurfaceKHR m_Surface;
            VkSwapchainKHR m_Swapchain;
            VkFormat m_ImageFormat;
            VkExtent2D m_Extent;
            std::vector<VkImage> m_Images;
            std::vector<VkImageView> m_ImageViews;
        };
    }
}
