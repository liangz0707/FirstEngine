#pragma once

#include "FirstEngine/Device/Export.h"
#include <vulkan/vulkan.h>
#include <vector>
#include "FirstEngine/Core/Window.h"

namespace FirstEngine {
    namespace Device {
        class VulkanRenderer;

        class FE_DEVICE_API Swapchain {
        public:
            Swapchain(VulkanRenderer* renderer);
            ~Swapchain();

            void Recreate();
            void Present(VkSemaphore imageAvailable, VkSemaphore renderFinished, VkFence inFlightFence);

            VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
            VkFormat GetImageFormat() const { return m_SwapchainImageFormat; }
            VkExtent2D GetExtent() const { return m_SwapchainExtent; }
            const std::vector<VkImage>& GetImages() const { return m_SwapchainImages; }
            const std::vector<VkImageView>& GetImageViews() const { return m_SwapchainImageViews; }

        private:
            void CreateSwapchain();
            void CreateImageViews();
            void CleanupSwapchain();

            VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
            VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
            VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

            VulkanRenderer* m_Renderer;
            Core::Window* m_Window;
            VkSwapchainKHR m_Swapchain;
            VkFormat m_SwapchainImageFormat;
            VkExtent2D m_SwapchainExtent;
            std::vector<VkImage> m_SwapchainImages;
            std::vector<VkImageView> m_SwapchainImageViews;
        };
    }
}
