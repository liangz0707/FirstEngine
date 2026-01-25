#include "FirstEngine/Renderer/GeometryPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IFramebuffer.h"

namespace FirstEngine {
    namespace Renderer {

        GeometryPass::GeometryPass()
            : IRenderPass("GeometryPass", RenderPassType::Geometry) {
        }

        void GeometryPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
            if (!pipeline) {
                return;
            }

            // Clear resources from previous frame (important: nodes are reused across frames)
            ClearResources();

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
            AttachmentResource materialRes(
                FrameGraphResourceNameToString(GBUFFER_MATERIAL),
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
            AddWriteResource(FrameGraphResourceNameToString(GBUFFER_MATERIAL), &materialRes);
            AddWriteResource(FrameGraphResourceNameToString(GBUFFER_DEPTH), &depthRes);
        }

        RenderCommandList GeometryPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            RenderCommandList cmdList;

            // Get render pass and framebuffer from builder
            RHI::IRenderPass* renderPass = builder.GetRenderPass();
            RHI::IFramebuffer* framebuffer = builder.GetFramebuffer();

            // Add BeginRenderPass command if we have valid render pass and framebuffer
            if (renderPass && framebuffer) {
                RenderCommand beginCmd;
                beginCmd.type = RenderCommandType::BeginRenderPass;
                beginCmd.params.beginRenderPass.renderPass = renderPass;
                beginCmd.params.beginRenderPass.framebuffer = framebuffer;
                beginCmd.params.beginRenderPass.width = framebuffer->GetWidth();
                beginCmd.params.beginRenderPass.height = framebuffer->GetHeight();
                // Clear colors for G-Buffer attachments (albedo, normal, material)
                beginCmd.params.beginRenderPass.clearColors = {0.0f, 0.0f, 0.0f, 0.0f,  // Albedo: black
                                                                0.0f, 0.0f, 0.0f, 0.0f,  // Normal: black
                                                                0.0f, 0.0f, 0.0f, 0.0f}; // Material: black
                beginCmd.params.beginRenderPass.clearDepth = 1.0f;  // Clear depth to 1.0 (far plane)
                beginCmd.params.beginRenderPass.clearStencil = 0;
                cmdList.AddCommand(beginCmd);
            }

            // Merge scene rendering commands into this pass
            // Scene commands contain BindPipeline, BindVertexBuffers, DrawIndexed, etc.
            if (sceneCommands && !sceneCommands->IsEmpty()) {
                const auto& sceneCmds = sceneCommands->GetCommands();
                for (const auto& cmd : sceneCmds) {
                    cmdList.AddCommand(cmd);
                }
            }

            // Add EndRenderPass command if we started a render pass
            if (renderPass && framebuffer) {
                RenderCommand endCmd;
                endCmd.type = RenderCommandType::EndRenderPass;
                cmdList.AddCommand(endCmd);
            }

            return cmdList;
        }

    } // namespace Renderer
} // namespace FirstEngine
