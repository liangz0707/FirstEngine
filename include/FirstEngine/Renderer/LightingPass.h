#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderPass.h"

namespace FirstEngine {
    namespace Renderer {

        // Lighting Pass for deferred rendering
        // Performs lighting calculation using G-Buffer
        class FE_RENDERER_API LightingPass : public IRenderPass {
        public:
            LightingPass();
            ~LightingPass() override = default;

            // IRenderPass interface
            void OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) override;
            RenderCommandList OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) override;

        private:
            // Resource names (using enum)
            static constexpr FrameGraphResourceName GBUFFER_ALBEDO = FrameGraphResourceName::GBufferAlbedo;
            static constexpr FrameGraphResourceName GBUFFER_NORMAL = FrameGraphResourceName::GBufferNormal;
            static constexpr FrameGraphResourceName GBUFFER_DEPTH = FrameGraphResourceName::GBufferDepth;
            static constexpr FrameGraphResourceName FINAL_OUTPUT = FrameGraphResourceName::FinalOutput;
        };

    } // namespace Renderer
} // namespace FirstEngine
