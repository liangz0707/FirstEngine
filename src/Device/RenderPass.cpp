#include "FirstEngine/Device/RenderPass.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        RenderPass::RenderPass(DeviceContext* context)
            : m_Context(context), m_RenderPass(VK_NULL_HANDLE) {
        }

        RenderPass::~RenderPass() {
            Destroy();
        }

        bool RenderPass::Create(const RenderPassDescription& description) {
            return Create(description.attachments, description.subpasses, description.dependencies);
        }

        bool RenderPass::Create(const std::vector<VkAttachmentDescription>& attachments,
                               const std::vector<VkSubpassDescription>& subpasses,
                               const std::vector<VkSubpassDependency>& dependencies) {
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
            renderPassInfo.pSubpasses = subpasses.data();
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            if (vkCreateRenderPass(m_Context->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        bool RenderPass::CreateSimple(VkFormat colorFormat, VkFormat depthFormat,
                                     VkSampleCountFlagBits samples) {
            std::vector<VkAttachmentDescription> attachments;

            // 颜色附件
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = colorFormat;
            colorAttachment.samples = samples;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = (samples == VK_SAMPLE_COUNT_1_BIT) 
                                         ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
                                         : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments.push_back(colorAttachment);

            // 深度附件（如果提供）
            VkAttachmentDescription depthAttachment{};
            bool hasDepth = (depthFormat != VK_FORMAT_UNDEFINED);
            if (hasDepth) {
                depthAttachment.format = depthFormat;
                depthAttachment.samples = samples;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                attachments.push_back(depthAttachment);
            }

            // 子通道
            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            if (hasDepth) {
                subpass.pDepthStencilAttachment = &depthAttachmentRef;
            }

            // 依赖关系
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            return Create(attachments, {subpass}, {dependency});
        }

        void RenderPass::Destroy() {
            if (m_RenderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(m_Context->GetDevice(), m_RenderPass, nullptr);
                m_RenderPass = VK_NULL_HANDLE;
            }
        }

    } // namespace Device
} // namespace FirstEngine
