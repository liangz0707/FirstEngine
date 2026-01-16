#include "FirstEngine/Renderer/CommandRecorder.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include <cstring>

namespace FirstEngine {
    namespace Renderer {

        CommandRecorder::CommandRecorder() = default;
        CommandRecorder::~CommandRecorder() = default;

        void CommandRecorder::RecordCommands(
            RHI::ICommandBuffer* commandBuffer,
            const RenderCommandList& commandList
        ) {
            if (!commandBuffer) {
                return;
            }

            const auto& commands = commandList.GetCommands();
            for (const auto& command : commands) {
                RecordCommand(commandBuffer, command);
            }
        }

        void CommandRecorder::RecordCommand(
            RHI::ICommandBuffer* commandBuffer,
            const RenderCommand& command
        ) {
            if (!commandBuffer) {
                return;
            }

            switch (command.type) {
                case RenderCommandType::BindPipeline:
                    RecordBindPipeline(commandBuffer, command.params.bindPipeline);
                    break;
                case RenderCommandType::BindDescriptorSets:
                    RecordBindDescriptorSets(commandBuffer, command.params.bindDescriptorSets);
                    break;
                case RenderCommandType::BindVertexBuffers:
                    RecordBindVertexBuffers(commandBuffer, command.params.bindVertexBuffers);
                    break;
                case RenderCommandType::BindIndexBuffer:
                    RecordBindIndexBuffer(commandBuffer, command.params.bindIndexBuffer);
                    break;
                case RenderCommandType::Draw:
                    RecordDraw(commandBuffer, command.params.draw);
                    break;
                case RenderCommandType::DrawIndexed:
                    RecordDrawIndexed(commandBuffer, command.params.drawIndexed);
                    break;
                case RenderCommandType::TransitionImageLayout:
                    RecordTransitionImageLayout(commandBuffer, command.params.transitionImageLayout);
                    break;
                case RenderCommandType::BeginRenderPass:
                    RecordBeginRenderPass(commandBuffer, command.params.beginRenderPass);
                    break;
                case RenderCommandType::EndRenderPass:
                    RecordEndRenderPass(commandBuffer, command.params.endRenderPass);
                    break;
                case RenderCommandType::PushConstants:
                    RecordPushConstants(commandBuffer, command.params.pushConstants);
                    break;
                default:
                    // Unknown command type, skip
                    break;
            }
        }

        void CommandRecorder::RecordBindPipeline(RHI::ICommandBuffer* cmd, const RenderCommand::BindPipelineParams& params) {
            if (params.pipeline) {
                cmd->BindPipeline(params.pipeline);
            }
        }

        void CommandRecorder::RecordBindDescriptorSets(RHI::ICommandBuffer* cmd, const RenderCommand::BindDescriptorSetsParams& params) {
            if (!params.descriptorSets.empty()) {
                cmd->BindDescriptorSets(params.firstSet, params.descriptorSets, params.dynamicOffsets);
            }
        }

        void CommandRecorder::RecordBindVertexBuffers(RHI::ICommandBuffer* cmd, const RenderCommand::BindVertexBuffersParams& params) {
            if (!params.buffers.empty()) {
                cmd->BindVertexBuffers(params.firstBinding, params.buffers, params.offsets);
            }
        }

        void CommandRecorder::RecordBindIndexBuffer(RHI::ICommandBuffer* cmd, const RenderCommand::BindIndexBufferParams& params) {
            if (params.buffer) {
                cmd->BindIndexBuffer(params.buffer, params.offset, params.is32Bit);
            }
        }

        void CommandRecorder::RecordDraw(RHI::ICommandBuffer* cmd, const RenderCommand::DrawParams& params) {
            cmd->Draw(params.vertexCount, params.instanceCount, params.firstVertex, params.firstInstance);
        }

        void CommandRecorder::RecordDrawIndexed(RHI::ICommandBuffer* cmd, const RenderCommand::DrawIndexedParams& params) {
            cmd->DrawIndexed(params.indexCount, params.instanceCount, params.firstIndex, params.vertexOffset, params.firstInstance);
        }

        void CommandRecorder::RecordTransitionImageLayout(RHI::ICommandBuffer* cmd, const RenderCommand::TransitionImageLayoutParams& params) {
            if (params.image) {
                cmd->TransitionImageLayout(params.image, params.formatOld, params.formatNew, params.mipLevels);
            }
        }

        void CommandRecorder::RecordBeginRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::BeginRenderPassParams& params) {
            if (params.renderPass && params.framebuffer) {
                cmd->BeginRenderPass(
                    params.renderPass,
                    params.framebuffer,
                    params.clearColors,
                    params.clearDepth,
                    params.clearStencil
                );
            }
        }

        void CommandRecorder::RecordEndRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::EndRenderPassParams& params) {
            (void)params; // Unused
            cmd->EndRenderPass();
        }

        void CommandRecorder::RecordPushConstants(RHI::ICommandBuffer* cmd, const RenderCommand::PushConstantsParams& params) {
            // Note: PushConstants is not in ICommandBuffer interface yet
            // This is a placeholder for future implementation
            (void)cmd;
            (void)params;
            // TODO: Implement when ICommandBuffer::PushConstants is added
        }

    } // namespace Renderer
} // namespace FirstEngine
