#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        // Geometry Pass for deferred rendering
        // Generates G-Buffer (Albedo, Normal, Depth)
        class FE_RENDERER_API GeometryPass : public IRenderPass {
        public:
            GeometryPass();
            ~GeometryPass() override = default;

            // IRenderPass interface
            void OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) override;
            RenderCommandList OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) override;

        private:
            // Resource names (using enum)
            static constexpr FrameGraphResourceName GBUFFER_ALBEDO = FrameGraphResourceName::GBufferAlbedo;
            static constexpr FrameGraphResourceName GBUFFER_NORMAL = FrameGraphResourceName::GBufferNormal;
            static constexpr FrameGraphResourceName GBUFFER_DEPTH = FrameGraphResourceName::GBufferDepth;
        };

    } // namespace Renderer
} // namespace FirstEngine
