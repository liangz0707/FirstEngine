#include "FirstEngine/Renderer/LightingPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include <iostream>

namespace FirstEngine {
    namespace Renderer {

        LightingPass::LightingPass()
            : IRenderPass("LightingPass", RenderPassType::Lighting) {
        }

        void LightingPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
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

        RenderCommandList LightingPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands)  {
            RenderCommandList cmdList;

            // Get render pass and framebuffer from builder
            RHI::IRenderPass* renderPass = builder.GetRenderPass();
            RHI::IFramebuffer* framebuffer = builder.GetFramebuffer();

            // Lighting Pass: Perform lighting calculation using G-Buffer
            // This pass performs deferred lighting by:
            // 1. Reading G-Buffer textures (Albedo, Normal, Depth)
            // 2. Rendering a fullscreen quad with a lighting shader
            // 3. Outputting the lit result to FinalOut put
            
            // Get G-Buffer resources from builder
            // IMPORTANT: Check resources BEFORE adding BeginRenderPass to avoid unbalanced commands
            RHI::IImage* gBufferAlbedo = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_ALBEDO));
            RHI::IImage* gBufferNormal = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_NORMAL));
            RHI::IImage* gBufferDepth = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_DEPTH));
            
            if (!gBufferAlbedo || !gBufferNormal || !gBufferDepth) {
                std::cerr << "Warning: LightingPass::OnDraw: G-Buffer resources not available" << std::endl;
                return cmdList; // Return empty command list if resources are not available
            }

            // Only add BeginRenderPass if we have valid render pass, framebuffer, AND resources
            if (renderPass && framebuffer) {
                RenderCommand beginCmd;
                beginCmd.type = RenderCommandType::BeginRenderPass;
                beginCmd.params.beginRenderPass.renderPass = renderPass;
                beginCmd.params.beginRenderPass.framebuffer = framebuffer;
                beginCmd.params.beginRenderPass.width = framebuffer->GetWidth();
                beginCmd.params.beginRenderPass.height = framebuffer->GetHeight();
                // Clear color for final output (black background)
                beginCmd.params.beginRenderPass.clearColors = {0.0f, 0.0f, 0.0f, 1.0f};
                beginCmd.params.beginRenderPass.clearDepth = 1.0f;
                beginCmd.params.beginRenderPass.clearStencil = 0;
                cmdList.AddCommand(beginCmd);
            }
            
            // TODO: Implement fullscreen quad rendering for lighting calculation
            // This requires:
            // 1. A fullscreen quad vertex buffer (or use Draw with 3 vertices for triangle strip)
            // 2. A lighting shader that:
            //    - Takes G-Buffer textures as input (Albedo, Normal, Depth)
            //    - Reconstructs world position from depth
            //    - Performs lighting calculations (ambient + directional light for now)
            //    - Outputs lit color to FinalOutput
            // 3. A graphics pipeline configured for fullscreen rendering:
            //    - No depth testing (or depth test disabled)
            //    - No vertex input (or minimal vertex input for fullscreen quad)
            //    - Fragment shader that samples G-Buffer and computes lighting
            
            // For now, we'll add a placeholder Draw command
            // In a full implementation, this would:
            // 1. Bind a lighting pipeline
            // 2. Bind G-Buffer textures as descriptor sets
            // 3. Draw fullscreen quad (3 vertices for triangle strip covering entire screen)
            // 4. Output lit result to FinalOutput
            
            // Example structure (commented out until fullscreen quad support is added):
            /*
            // Bind lighting pipeline
            RenderCommand bindPipelineCmd;
            bindPipelineCmd.type = RenderCommandType::BindPipeline;
            bindPipelineCmd.params.bindPipeline.pipeline = lightingPipeline;
            cmdList.AddCommand(bindPipelineCmd);
            
            // Bind G-Buffer textures as descriptor sets
            RenderCommand bindDescCmd;
            bindDescCmd.type = RenderCommandType::BindDescriptorSets;
            bindDescCmd.params.bindDescriptorSets.firstSet = 0;
            bindDescCmd.params.bindDescriptorSets.descriptorSets = {gBufferDescriptorSet};
            cmdList.AddCommand(bindDescCmd);
            
            // Draw fullscreen quad (3 vertices for triangle strip)
            RenderCommand drawCmd;
            drawCmd.type = RenderCommandType::Draw;
            drawCmd.params.draw.vertexCount = 3;
            drawCmd.params.draw.instanceCount = 1;
            drawCmd.params.draw.firstVertex = 0;
            drawCmd.params.draw.firstInstance = 0;
            cmdList.AddCommand(drawCmd);
            */
            
            // Lighting pass doesn't use scene commands

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
