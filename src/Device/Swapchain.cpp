#include "FirstEngine/Device/Swapchain.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <algorithm>
#include <stdexcept>

namespace FirstEngine {
    namespace Device {
        Swapchain::Swapchain(DeviceContext* context, Core::Window* window, VkSurfaceKHR surface)
            : m_Context(context), m_Window(window), m_Surface(surface),
              m_Swapchain(VK_NULL_HANDLE), m_ImageFormat(VK_FORMAT_UNDEFINED) {
            m_Extent = {0, 0};
        }

        Swapchain::~Swapchain() {
            Destroy();
        }

        bool Swapchain::Create() {
            CreateSwapchain();
            CreateImageViews();
            return IsValid();
        }

        bool Swapchain::Recreate() {
            CleanupSwapchain();
            return Create();
        }

        bool Swapchain::Present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            
            if (waitSemaphore != VK_NULL_HANDLE) {
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = &waitSemaphore;
            }

            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_Swapchain;
            presentInfo.pImageIndices = &imageIndex;

            return vkQueuePresentKHR(m_Context->GetPresentQueue(), &presentInfo) == VK_SUCCESS;
        }

        bool Swapchain::AcquireNextImage(VkSemaphore semaphore, VkFence fence, uint32_t& imageIndex) {
            return vkAcquireNextImageKHR(m_Context->GetDevice(), m_Swapchain, UINT64_MAX,
                                         semaphore, fence, &imageIndex) == VK_SUCCESS;
        }

        void Swapchain::Destroy() {
            CleanupSwapchain();
        }

        void Swapchain::CreateSwapchain() {
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context->GetPhysicalDevice(), m_Surface, &capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(), m_Surface, &formatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(), m_Surface, &formatCount, formats.data());

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

            VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(formats);
            VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
            VkExtent2D extent = ChooseSwapExtent(capabilities);

            uint32_t imageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = m_Surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = {m_Context->GetGraphicsQueueFamily(), m_Context->GetPresentQueueFamily()};
            if (m_Context->GetGraphicsQueueFamily() != m_Context->GetPresentQueueFamily()) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0;
                createInfo.pQueueFamilyIndices = nullptr;
            }

            createInfo.preTransform = capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(m_Context->GetDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create swap chain!");
            }

            vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &imageCount, nullptr);
            m_Images.resize(imageCount);
            vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &imageCount, m_Images.data());

            m_ImageFormat = surfaceFormat.format;
            m_Extent = extent;
        }

        void Swapchain::CreateImageViews() {
            m_ImageViews.resize(m_Images.size());

            for (size_t i = 0; i < m_Images.size(); i++) {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = m_Images[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = m_ImageFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(m_Context->GetDevice(), &createInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create image views!");
                }
            }
        }

        void Swapchain::CleanupSwapchain() {
            for (auto imageView : m_ImageViews) {
                vkDestroyImageView(m_Context->GetDevice(), imageView, nullptr);
            }
            m_ImageViews.clear();

            if (m_Swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_Context->GetDevice(), m_Swapchain, nullptr);
                m_Swapchain = VK_NULL_HANDLE;
            }
        }

        VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

        VkPresentModeKHR Swapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != UINT32_MAX) {
                return capabilities.currentExtent;
            } else {
                int width, height;
                m_Window->GetFramebufferSize(&width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

    } // namespace Device
} // namespace FirstEngine
