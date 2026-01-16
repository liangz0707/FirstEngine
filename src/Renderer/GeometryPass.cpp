#include "FirstEngine/Renderer/GeometryPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IDevice.h"

namespace FirstEngine {
    namespace Renderer {

        GeometryPass::GeometryPass()
            : IRenderPass("GeometryPass", RenderPassType::Geometry) {
        }

        void GeometryPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
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
            // After this, AddWriteResource will automatically add and allocate resources
            // Execute callback is automatically set from OnDraw() in AddNode
            if (!frameGraph.AddNode(this)) {
                return; // Failed to add node
            }

            // Create SceneRenderer for this pass (GeometryPass renders opaque objects)
            // Get device from pipeline
            RHI::IDevice* device = pipeline->GetDevice();
            if (device) {
                auto sceneRenderer = std::make_unique<SceneRenderer>(device);
                sceneRenderer->SetRenderFlags(RenderObjectFlag::Opaque);
                SetSceneRenderer(std::move(sceneRenderer));
            }

            // Declare resource access (automatically adds and allocates resources)
            // Use AttachmentResource constructor for cleaner code
            AttachmentResource albedoRes(
                FrameGraphResourceNameToString(GBUFFER_ALBEDO),
                resolution.width,
                resolution.height,
                RHI::Format::R8G8B8A8_UNORM,
                false
            );
            AttachmentResource normalRes(
                FrameGraphResourceNameToString(GBUFFER_NORMAL),
                resolution.width,
                resolution.height,
                RHI::Format::R8G8B8A8_UNORM,
                false
            );
            AttachmentResource depthRes(
                FrameGraphResourceNameToString(GBUFFER_DEPTH),
                resolution.width,
                resolution.height,
                RHI::Format::D32_SFLOAT,
                true
            );

            AddWriteResource(FrameGraphResourceNameToString(GBUFFER_ALBEDO), &albedoRes);
            AddWriteResource(FrameGraphResourceNameToString(GBUFFER_NORMAL), &normalRes);
            AddWriteResource(FrameGraphResourceNameToString(GBUFFER_DEPTH), &depthRes);
        }

        RenderCommandList GeometryPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            RenderCommandList cmdList;

            // TODO: Add BeginRenderPass command for G-Buffer
            // TODO: Add resource layout transitions if needed

            // Merge scene rendering commands into this pass
            // Scene commands contain BindPipeline, BindVertexBuffers, DrawIndexed, etc.
            if (sceneCommands && !sceneCommands->IsEmpty()) {
                const auto& sceneCmds = sceneCommands->GetCommands();
                for (const auto& cmd : sceneCmds) {
                    cmdList.AddCommand(cmd);
                }
            }

            // TODO: Add EndRenderPass command

            return cmdList;
        }

    } // namespace Renderer
} // namespace FirstEngine
