#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Forward declarations
        class RenderPass;
        class RenderTarget;
        class Image;

        // Framebuffer wrapper
        class FE_DEVICE_API Framebuffer {
        public:
            Framebuffer(DeviceContext* context);
            ~Framebuffer();

            // Create framebuffer (from image views)
            bool Create(RenderPass* renderPass, uint32_t width, uint32_t height,
                       const std::vector<VkImageView>& attachments, uint32_t layers = 1);

            // Create framebuffer (from render target)
            bool Create(RenderPass* renderPass, RenderTarget* renderTarget);

            // Create framebuffer (from swapchain image views)
            bool Create(RenderPass* renderPass, uint32_t width, uint32_t height,
                       VkImageView colorImageView, VkImageView depthImageView = VK_NULL_HANDLE);

            // Destroy framebuffer
            void Destroy();

            VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            bool IsValid() const { return m_Framebuffer != VK_NULL_HANDLE; }

        private:
            DeviceContext* m_Context;
            VkFramebuffer m_Framebuffer;
            uint32_t m_Width;
            uint32_t m_Height;
        };

    } // namespace Device
} // namespace FirstEngine
