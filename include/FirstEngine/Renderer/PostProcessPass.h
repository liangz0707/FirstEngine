#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderPass.h"

namespace FirstEngine {
    namespace Renderer {

        // Post-Process Pass for deferred rendering
        // Applies post-processing effects to the final output
        class FE_RENDERER_API PostProcessPass : public IRenderPass {
        public:
            PostProcessPass();
            ~PostProcessPass() override = default;

            // IRenderPass interface
            void OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) override;
            RenderCommandList OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) override;

        private:
            // Resource names (using enum)
            static constexpr FrameGraphResourceName FINAL_OUTPUT = FrameGraphResourceName::FinalOutput;
        };

    } // namespace Renderer
} // namespace FirstEngine
