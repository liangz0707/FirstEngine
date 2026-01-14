#include "FirstEngine/Device/RenderTarget.h"
#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        RenderTarget::RenderTarget(DeviceContext* context)
            : m_Context(context), m_Width(0), m_Height(0),
              m_ColorFormat(VK_FORMAT_UNDEFINED), m_DepthFormat(VK_FORMAT_UNDEFINED) {
        }

        RenderTarget::~RenderTarget() {
            Destroy();
        }

        bool RenderTarget::Create(const RenderTargetDescription& description) {
            return Create(description.width, description.height, description.colorFormat,
                         description.depthFormat, description.samples);
        }

        bool RenderTarget::Create(uint32_t width, uint32_t height, VkFormat colorFormat,
                                 VkFormat depthFormat, VkSampleCountFlagBits samples) {
            m_Width = width;
            m_Height = height;
            m_ColorFormat = colorFormat;
            m_DepthFormat = depthFormat;

            MemoryManager memoryManager(m_Context);

            // 创建颜色附件
            auto colorImage = memoryManager.CreateImage(
                width, height, 1, samples, colorFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            if (!colorImage) {
                return false;
            }

            if (!colorImage->CreateImageView(VK_IMAGE_VIEW_TYPE_2D, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT)) {
                return false;
            }

            m_ColorAttachments.push_back(std::move(colorImage));

            // 创建深度附件（如果提供）
            if (depthFormat != VK_FORMAT_UNDEFINED) {
                auto depthImage = memoryManager.CreateImage(
                    width, height, 1, samples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );

                if (!depthImage) {
                    return false;
                }

                if (!depthImage->CreateImageView(VK_IMAGE_VIEW_TYPE_2D, depthFormat, 
                                                VK_IMAGE_ASPECT_DEPTH_BIT)) {
                    return false;
                }

                m_DepthAttachment = std::move(depthImage);
            }

            return true;
        }

        void RenderTarget::Destroy() {
            m_ColorAttachments.clear();
            m_DepthAttachment.reset();
            m_Width = 0;
            m_Height = 0;
        }

        Image* RenderTarget::GetColorAttachment(uint32_t index) const {
            if (index >= m_ColorAttachments.size()) {
                return nullptr;
            }
            return m_ColorAttachments[index].get();
        }

    } // namespace Device
} // namespace FirstEngine
