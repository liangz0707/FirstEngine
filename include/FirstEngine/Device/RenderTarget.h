#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include "FirstEngine/Device/MemoryManager.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Render target description
        FE_DEVICE_API struct RenderTargetDescription {
            uint32_t width;
            uint32_t height;
            VkFormat colorFormat;
            VkFormat depthFormat;
            VkSampleCountFlagBits samples;
            uint32_t colorAttachmentCount;
        };

        // Render target wrapper
        class FE_DEVICE_API RenderTarget {
        public:
            RenderTarget(DeviceContext* context);
            ~RenderTarget();

            // Create render target
            bool Create(const RenderTargetDescription& description);

            // Create simple render target
            bool Create(uint32_t width, uint32_t height, VkFormat colorFormat,
                       VkFormat depthFormat = VK_FORMAT_UNDEFINED,
                       VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

            // Destroy render target
            void Destroy();

            // Get color attachments
            const std::vector<std::unique_ptr<Image>>& GetColorAttachments() const { return m_ColorAttachments; }
            Image* GetColorAttachment(uint32_t index = 0) const;

            // Get depth attachment
            Image* GetDepthAttachment() const { return m_DepthAttachment.get(); }

            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            VkFormat GetColorFormat() const { return m_ColorFormat; }
            VkFormat GetDepthFormat() const { return m_DepthFormat; }

        private:
            DeviceContext* m_Context;
            uint32_t m_Width;
            uint32_t m_Height;
            VkFormat m_ColorFormat;
            VkFormat m_DepthFormat;
            std::vector<std::unique_ptr<Image>> m_ColorAttachments;
            std::unique_ptr<Image> m_DepthAttachment;
        };

    } // namespace Device
} // namespace FirstEngine
