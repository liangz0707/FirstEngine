#include "FirstEngine/Renderer/PostProcessPass.h"
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

        PostProcessPass::PostProcessPass()
            : IRenderPass("PostProcessPass", RenderPassType::PostProcess) {
        }

        void PostProcessPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
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

            // Get render pass and framebuffer from builder
            RHI::IRenderPass* renderPass = builder.GetRenderPass();
            RHI::IFramebuffer* framebuffer = builder.GetFramebuffer();

            // Post-process Pass: Post-processing
            // This pass applies post-processing effects to the final output by:
            // 1. Reading the FinalOutput texture (lit result from LightingPass)
            // 2. Rendering a fullscreen quad with post-processing shader
            // 3. Outputting the processed result back to FinalOutput (or a separate buffer)
            
            // Get final output resource from builder
            // IMPORTANT: Check resources BEFORE adding BeginRenderPass to avoid unbalanced commands
            RHI::IImage* finalOutput = builder.ReadTexture(FrameGraphResourceNameToString(FINAL_OUTPUT));
            
            if (!finalOutput) {
                std::cerr << "Warning: PostProcessPass::OnDraw: FinalOutput resource not available" << std::endl;
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
                // Clear color for post-process output (no clear, load existing)
                beginCmd.params.beginRenderPass.clearColors = {0.0f, 0.0f, 0.0f, 1.0f};
                beginCmd.params.beginRenderPass.clearDepth = 1.0f;
                beginCmd.params.beginRenderPass.clearStencil = 0;
                cmdList.AddCommand(beginCmd);
            }
            
            // TODO: Implement fullscreen quad rendering for post-processing
            // This requires:
            // 1. A fullscreen quad vertex buffer (or use Draw with 3 vertices for triangle strip)
            // 2. A post-processing shader that:
            //    - Takes FinalOutput texture as input
            //    - Applies post-processing effects (tone mapping, bloom, FXAA, etc.)
            //    - Outputs processed color to FinalOutput
            // 3. A graphics pipeline configured for fullscreen rendering:
            //    - No depth testing
            //    - No vertex input (or minimal vertex input for fullscreen quad)
            //    - Fragment shader that samples input texture and applies effects
            
            // For now, we'll add a placeholder
            // In a full implementation, this would:
            // 1. Bind a post-processing pipeline
            // 2. Bind FinalOutput texture as descriptor set
            // 3. Draw fullscreen quad (3 vertices for triangle strip covering entire screen)
            // 4. Output processed result to FinalOutput
            
            // Example structure (commented out until fullscreen quad support is added):
            /*
            // Bind post-processing pipeline
            RenderCommand bindPipelineCmd;
            bindPipelineCmd.type = RenderCommandType::BindPipeline;
            bindPipelineCmd.params.bindPipeline.pipeline = postProcessPipeline;
            cmdList.AddCommand(bindPipelineCmd);
            
            // Bind FinalOutput texture as descriptor set
            RenderCommand bindDescCmd;
            bindDescCmd.type = RenderCommandType::BindDescriptorSets;
            bindDescCmd.params.bindDescriptorSets.firstSet = 0;
            bindDescCmd.params.bindDescriptorSets.descriptorSets = {finalOutputDescriptorSet};
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
            
            // Post-process pass doesn't use scene commands

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
