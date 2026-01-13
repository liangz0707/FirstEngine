#include "FirstEngine/Device/Swapchain.h"
#include "FirstEngine/Device/VulkanRenderer.h"
#include <algorithm>
#include <stdexcept>

namespace FirstEngine {
    namespace Device {
        Swapchain::Swapchain(VulkanRenderer* renderer)
            : m_Renderer(renderer), m_Swapchain(VK_NULL_HANDLE) {
            
            m_Window = renderer->GetWindow();
            CreateSwapchain();
            CreateImageViews();
        }

        Swapchain::~Swapchain() {
            CleanupSwapchain();
        }

        void Swapchain::CreateSwapchain() {
            VkSurfaceKHR surface = m_Renderer->GetSurface();
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Renderer->GetPhysicalDevice(), surface, &capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Renderer->GetPhysicalDevice(), surface, &formatCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Renderer->GetPhysicalDevice(), surface, &formatCount, formats.data());

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Renderer->GetPhysicalDevice(), surface, &presentModeCount, nullptr);
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Renderer->GetPhysicalDevice(), surface, &presentModeCount, presentModes.data());

            VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(formats);
            VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
            VkExtent2D extent = ChooseSwapExtent(capabilities);

            uint32_t imageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = {0, 1}; // Simplified - should query actual indices
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;

            createInfo.preTransform = capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(m_Renderer->GetDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create swap chain!");
            }

            vkGetSwapchainImagesKHR(m_Renderer->GetDevice(), m_Swapchain, &imageCount, nullptr);
            m_SwapchainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(m_Renderer->GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

            m_SwapchainImageFormat = surfaceFormat.format;
            m_SwapchainExtent = extent;
        }

        void Swapchain::CreateImageViews() {
            m_SwapchainImageViews.resize(m_SwapchainImages.size());

            for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = m_SwapchainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = m_SwapchainImageFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(m_Renderer->GetDevice(), &createInfo, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create image views!");
                }
            }
        }

        void Swapchain::Recreate() {
            vkDeviceWaitIdle(m_Renderer->GetDevice());
            CleanupSwapchain();
            CreateSwapchain();
            CreateImageViews();
        }

        void Swapchain::Present(VkSemaphore imageAvailable, VkSemaphore renderFinished, VkFence inFlightFence) {
            vkWaitForFences(m_Renderer->GetDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(m_Renderer->GetDevice(), m_Swapchain, UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                Recreate();
                return;
            } else if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to acquire swap chain image!");
            }

            vkResetFences(m_Renderer->GetDevice(), 1, &inFlightFence);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailable};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 0; // Will be set by pipeline
            submitInfo.pCommandBuffers = nullptr;

            VkSemaphore signalSemaphores[] = {renderFinished};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            if (vkQueueSubmit(m_Renderer->GetGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("Failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {m_Swapchain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr;

            result = vkQueuePresentKHR(m_Renderer->GetPresentQueue(), &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                Recreate();
            } else if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to present swap chain image!");
            }
        }

        void Swapchain::CleanupSwapchain() {
            for (auto imageView : m_SwapchainImageViews) {
                vkDestroyImageView(m_Renderer->GetDevice(), imageView, nullptr);
            }
            if (m_Swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_Renderer->GetDevice(), m_Swapchain, nullptr);
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
                glfwGetFramebufferSize(m_Window->GetHandle(), &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }
    }
}
