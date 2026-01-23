#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include "FirstEngine/RHI/IImage.h"  // IImageView is defined in IImage.h
#include "FirstEngine/RHI/IRenderPass.h"
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        // Wrapper for IFramebuffer to manage lifecycle through IRenderResource
        class FE_RENDERER_API FramebufferResource : public IRenderResource {
        public:
            FramebufferResource();
            ~FramebufferResource() override;

            void SetFramebuffer(RHI::IFramebuffer* framebuffer);
            RHI::IFramebuffer* GetFramebuffer() const { return m_Framebuffer; }

        protected:
            bool DoCreate(RHI::IDevice* device) override;
            bool DoUpdate(RHI::IDevice* device) override;
            void DoDestroy() override;

        private:
            RHI::IFramebuffer* m_Framebuffer = nullptr;
        };

        // Wrapper for IRenderPass to manage lifecycle through IRenderResource
        class FE_RENDERER_API RenderPassResource : public IRenderResource {
        public:
            RenderPassResource();
            ~RenderPassResource() override;

            void SetRenderPass(RHI::IRenderPass* renderPass);
            RHI::IRenderPass* GetRenderPass() const { return m_RenderPass; }

        protected:
            bool DoCreate(RHI::IDevice* device) override;
            bool DoUpdate(RHI::IDevice* device) override;
            void DoDestroy() override;

        private:
            RHI::IRenderPass* m_RenderPass = nullptr;
        };

    } // namespace Renderer
} // namespace FirstEngine
