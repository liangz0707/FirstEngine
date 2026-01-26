#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/Element.h"
#include <memory>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {
        class BuiltinGeometry;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Post-Process Pass for deferred rendering
        // Applies post-processing effects using Ping-Pong Buffer pattern:
        // - Reads from FINAL_OUTPUT (output from LightingPass)
        // - Writes to PostProcessBuffer (separate output resource)
        // This avoids Vulkan limitation of not being able to use the same image
        // as both attachment (write) and shader resource (read) in the same render pass
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
            
            // Element system for fullscreen quad rendering
            std::unique_ptr<Resources::BuiltinGeometry> m_QuadGeometry;
        };

    } // namespace Renderer
} // namespace FirstEngine
