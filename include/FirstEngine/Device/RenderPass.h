#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Render pass description
        FE_DEVICE_API struct RenderPassDescription {
            std::vector<VkAttachmentDescription> attachments;
            std::vector<VkSubpassDescription> subpasses;
            std::vector<VkSubpassDependency> dependencies;
        };

        // Render pass wrapper
        class FE_DEVICE_API RenderPass {
        public:
            RenderPass(DeviceContext* context);
            ~RenderPass();

            // Create render pass
            bool Create(const RenderPassDescription& description);
            bool Create(const std::vector<VkAttachmentDescription>& attachments,
                      const std::vector<VkSubpassDescription>& subpasses,
                      const std::vector<VkSubpassDependency>& dependencies = {});

            // Create simple render pass (for swapchain)
            bool CreateSimple(VkFormat colorFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED,
                             VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

            // Destroy render pass
            void Destroy();

            VkRenderPass GetRenderPass() const { return m_RenderPass; }
            bool IsValid() const { return m_RenderPass != VK_NULL_HANDLE; }

        private:
            DeviceContext* m_Context;
            VkRenderPass m_RenderPass;
        };

    } // namespace Device
} // namespace FirstEngine
