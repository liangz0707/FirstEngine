#include "FirstEngine/Device/Framebuffer.h"
#include "FirstEngine/Device/RenderPass.h"
#include "FirstEngine/Device/RenderTarget.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        Framebuffer::Framebuffer(DeviceContext* context)
            : m_Context(context), m_Framebuffer(VK_NULL_HANDLE), m_Width(0), m_Height(0) {
        }

        Framebuffer::~Framebuffer() {
            Destroy();
        }

        bool Framebuffer::Create(RenderPass* renderPass, uint32_t width, uint32_t height,
                                const std::vector<VkImageView>& attachments, uint32_t layers) {
            m_Width = width;
            m_Height = height;

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass->GetRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = layers;

            if (vkCreateFramebuffer(m_Context->GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        bool Framebuffer::Create(RenderPass* renderPass, RenderTarget* renderTarget) {
            std::vector<VkImageView> attachments;

            // 添加颜色附件
            for (uint32_t i = 0; i < renderTarget->GetColorAttachments().size(); ++i) {
                Image* colorImage = renderTarget->GetColorAttachment(i);
                if (colorImage) {
                    attachments.push_back(colorImage->GetImageView());
                }
            }

            // 添加深度附件
            if (renderTarget->GetDepthAttachment()) {
                attachments.push_back(renderTarget->GetDepthAttachment()->GetImageView());
            }

            return Create(renderPass, renderTarget->GetWidth(), renderTarget->GetHeight(), attachments);
        }

        bool Framebuffer::Create(RenderPass* renderPass, uint32_t width, uint32_t height,
                                VkImageView colorImageView, VkImageView depthImageView) {
            std::vector<VkImageView> attachments;
            attachments.push_back(colorImageView);

            if (depthImageView != VK_NULL_HANDLE) {
                attachments.push_back(depthImageView);
            }

            return Create(renderPass, width, height, attachments);
        }

        void Framebuffer::Destroy() {
            if (m_Framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_Context->GetDevice(), m_Framebuffer, nullptr);
                m_Framebuffer = VK_NULL_HANDLE;
            }
            m_Width = 0;
            m_Height = 0;
        }

    } // namespace Device
} // namespace FirstEngine
