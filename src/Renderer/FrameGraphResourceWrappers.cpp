#include "FirstEngine/Renderer/FrameGraphResourceWrappers.h"
#include "FirstEngine/RHI/IDevice.h"
#include <iostream>

namespace FirstEngine {
    namespace Renderer {

        FramebufferResource::FramebufferResource() {
        }

        FramebufferResource::~FramebufferResource() {
            if (m_Framebuffer && IsCreated()) {
                DoDestroy();
            }
        }

        void FramebufferResource::SetFramebuffer(RHI::IFramebuffer* framebuffer) {
            // If we already have a framebuffer and it's different, schedule destroy for the old one
            if (m_Framebuffer && m_Framebuffer != framebuffer && IsCreated()) {
                ScheduleDestroy();
            }
            m_Framebuffer = framebuffer;
            if (framebuffer) {
                // Mark as created (framebuffer is already created externally)
                SetResourceState(ResourceState::Created);
            }
        }

        bool FramebufferResource::DoCreate(RHI::IDevice* device) {
            // Framebuffer is already created externally, just mark as created
            if (m_Framebuffer) {
                return true;
            }
            return false;
        }

        bool FramebufferResource::DoUpdate(RHI::IDevice* device) {
            // Framebuffers don't need updates
            return true;
        }

        void FramebufferResource::DoDestroy() {
            if (m_Framebuffer) {
                // Framebuffer is managed by unique_ptr in VulkanDevice, but we got it via release()
                // So we need to delete it directly - the destructor will handle Vulkan cleanup
                delete m_Framebuffer;
                m_Framebuffer = nullptr;
            }
        }

        RenderPassResource::RenderPassResource() {
        }

        RenderPassResource::~RenderPassResource() {
            if (m_RenderPass && IsCreated()) {
                DoDestroy();
            }
        }

        void RenderPassResource::SetRenderPass(RHI::IRenderPass* renderPass) {
            // If we already have a render pass and it's different, schedule destroy for the old one
            if (m_RenderPass && m_RenderPass != renderPass && IsCreated()) {
                ScheduleDestroy();
            }
            m_RenderPass = renderPass;
            if (renderPass) {
                SetResourceState(ResourceState::Created);
            }
        }

        bool RenderPassResource::DoCreate(RHI::IDevice* device) {
            // RenderPass is already created externally, just mark as created
            if (m_RenderPass) {
                return true;
            }
            return false;
        }

        bool RenderPassResource::DoUpdate(RHI::IDevice* device) {
            // RenderPasses don't need updates
            return true;
        }

        void RenderPassResource::DoDestroy() {
            if (m_RenderPass) {
                // RenderPass is managed by unique_ptr in VulkanDevice, but we got it via release()
                // So we need to delete it directly - the destructor will handle Vulkan cleanup
                delete m_RenderPass;
                m_RenderPass = nullptr;
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
