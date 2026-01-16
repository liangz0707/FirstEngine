#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/GeometryPass.h"
#include "FirstEngine/Renderer/LightingPass.h"
#include "FirstEngine/Renderer/PostProcessPass.h"
#include "FirstEngine/RHI/Types.h"
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // Deferred rendering pipeline implementation
        // Implements G-Buffer based deferred rendering
        // Uses IRenderPass classes to manage each pass
        // Each pass manages its own resource allocation
        class FE_RENDERER_API DeferredRenderPipeline : public IRenderPipeline {
        public:
            DeferredRenderPipeline(RHI::IDevice* device);
            ~DeferredRenderPipeline() override;

            // IRenderPipeline interface
            std::string GetName() const override { return "DeferredRendering"; }
            bool BuildFrameGraph(FrameGraph& frameGraph, const RenderConfig& config) override;

            // Get RenderConfig (for Pass access)
            const RenderConfig& GetRenderConfig() const { return m_RenderConfig; }
            RenderConfig& GetRenderConfig() { return m_RenderConfig; }

        private:
            // Render configuration (stored as member to avoid parameter passing)
            RenderConfig m_RenderConfig;

            // Render pass instances
            std::unique_ptr<GeometryPass> m_GeometryPass;
            std::unique_ptr<LightingPass> m_LightingPass;
            std::unique_ptr<PostProcessPass> m_PostProcessPass;
        };

    } // namespace Renderer
} // namespace FirstEngine
