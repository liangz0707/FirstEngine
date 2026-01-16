#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/RHI/ICommandBuffer.h"

namespace FirstEngine {
    namespace Renderer {

        // Command recorder - converts RenderCommandList to actual CommandBuffer commands
        // This provides the bridge between data structures and GPU command recording
        class FE_RENDERER_API CommandRecorder {
        public:
            CommandRecorder();
            ~CommandRecorder();

            // Record a RenderCommandList to a CommandBuffer
            // The CommandBuffer should already be in recording state (Begin() called)
            void RecordCommands(
                RHI::ICommandBuffer* commandBuffer,
                const RenderCommandList& commandList
            );

            // Record a single command
            void RecordCommand(
                RHI::ICommandBuffer* commandBuffer,
                const RenderCommand& command
            );

        private:
            // Helper methods for each command type
            void RecordBindPipeline(RHI::ICommandBuffer* cmd, const RenderCommand::BindPipelineParams& params);
            void RecordBindDescriptorSets(RHI::ICommandBuffer* cmd, const RenderCommand::BindDescriptorSetsParams& params);
            void RecordBindVertexBuffers(RHI::ICommandBuffer* cmd, const RenderCommand::BindVertexBuffersParams& params);
            void RecordBindIndexBuffer(RHI::ICommandBuffer* cmd, const RenderCommand::BindIndexBufferParams& params);
            void RecordDraw(RHI::ICommandBuffer* cmd, const RenderCommand::DrawParams& params);
            void RecordDrawIndexed(RHI::ICommandBuffer* cmd, const RenderCommand::DrawIndexedParams& params);
            void RecordTransitionImageLayout(RHI::ICommandBuffer* cmd, const RenderCommand::TransitionImageLayoutParams& params);
            void RecordBeginRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::BeginRenderPassParams& params);
            void RecordEndRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::EndRenderPassParams& params);
            void RecordPushConstants(RHI::ICommandBuffer* cmd, const RenderCommand::PushConstantsParams& params);
        };

    } // namespace Renderer
} // namespace FirstEngine
