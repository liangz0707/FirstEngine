#include "FirstEngine/Renderer/LightingPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        LightingPass::LightingPass()
            : IRenderPass("LightingPass", RenderPassType::Lighting) {
        }

        void LightingPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
            if (!pipeline) {
                return;
            }

            // Get RenderConfig from pipeline
            auto* deferredPipeline = dynamic_cast<DeferredRenderPipeline*>(pipeline);
            if (!deferredPipeline) {
                return;
            }
            const auto& resolution = deferredPipeline->GetRenderConfig().GetResolution();

            // Add self to FrameGraph (this Pass IS the Node)
            if (!frameGraph.AddNode(this)) {
                return; // Failed to add node
            }

            // Declare resource access (automatically adds and allocates resources if needed)
            // Read G-Buffer resources (no description needed, they should already exist)
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_ALBEDO));
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_NORMAL));
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_DEPTH));

            // Write to final output (with description, will auto-add and allocate if not exists)
            AttachmentResource outputRes(
                FrameGraphResourceNameToString(FINAL_OUTPUT),
                resolution.width,
                resolution.height,
                RHI::Format::B8G8R8A8_UNORM,
                false
            );
            AddWriteResource(FrameGraphResourceNameToString(FINAL_OUTPUT), &outputRes);
        }

        RenderCommandList LightingPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            RenderCommandList cmdList;

            // Lighting Pass: Perform lighting calculation using G-Buffer
            // TODO: Implement lighting calculation
            // Lighting pass doesn't use scene commands

            return cmdList;
        }

    } // namespace Renderer
} // namespace FirstEngine
