#include "FirstEngine/Renderer/PostProcessPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        PostProcessPass::PostProcessPass()
            : IRenderPass("PostProcessPass", RenderPassType::PostProcess) {
        }

        void PostProcessPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
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
            // Read and write to final output (in-place post-processing)
            AttachmentResource outputRes(
                FrameGraphResourceNameToString(FINAL_OUTPUT),
                resolution.width,
                resolution.height,
                RHI::Format::B8G8R8A8_UNORM,
                false
            );
            AddReadResource(FrameGraphResourceNameToString(FINAL_OUTPUT), &outputRes);
            AddWriteResource(FrameGraphResourceNameToString(FINAL_OUTPUT), &outputRes);
        }

        RenderCommandList PostProcessPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            RenderCommandList cmdList;

            // Post-process Pass: Post-processing
            // TODO: Implement post-processing
            // Post-process pass doesn't use scene commands

            return cmdList;
        }

    } // namespace Renderer
} // namespace FirstEngine
