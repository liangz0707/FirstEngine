#include "FirstEngine/Renderer/CommandRecorder.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include <cstring>
#include <iostream>

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

            // Track render pass state to ensure BeginRenderPass and EndRenderPass are properly matched
            int renderPassDepth = 0;
            bool skipUntilEndRenderPass = false; // Skip commands if BeginRenderPass failed

            const auto& commands = commandList.GetCommands();
            for (size_t i = 0; i < commands.size(); ++i) {
                const auto& command = commands[i];
                
                // If we're skipping commands due to a failed BeginRenderPass, only process EndRenderPass
                if (skipUntilEndRenderPass && command.type != RenderCommandType::EndRenderPass) {
                    continue;
                }
                
                // Pass current depth to RecordCommand (before updating depth)
                bool commandSucceeded = RecordCommand(commandBuffer, command, renderPassDepth);

                // Update render pass depth after recording the command
                // Only increase depth if BeginRenderPass actually succeeded
                if (command.type == RenderCommandType::BeginRenderPass) {
                    if (commandSucceeded) {
                        renderPassDepth++;
                        skipUntilEndRenderPass = false; // Reset skip flag on success
                    } else {
                        std::cerr << "CommandRecorder: BeginRenderPass FAILED at command index " << i 
                                  << ", depth remains: " << renderPassDepth << std::endl;
                        std::cerr << "CommandRecorder: Skipping all commands until EndRenderPass" << std::endl;
                        skipUntilEndRenderPass = true; // Skip all commands until EndRenderPass
                    }
                } else if (command.type == RenderCommandType::EndRenderPass) {
                    if (renderPassDepth > 0) {
                        renderPassDepth--;
                        skipUntilEndRenderPass = false; // Reset skip flag
                    } else {
                        std::cerr << "CommandRecorder: EndRenderPass called but renderPassDepth is 0 (no active render pass)" << std::endl;
                    }
                }
            }

            // Warn if render pass depth is not balanced
            if (renderPassDepth != 0) {
                std::cerr << "Warning: CommandRecorder: Unbalanced BeginRenderPass/EndRenderPass commands. "
                          << "Depth: " << renderPassDepth << std::endl;
            }
        }

        bool CommandRecorder::RecordCommand(
            RHI::ICommandBuffer* commandBuffer,
            const RenderCommand& command,
            int renderPassDepth
        ) {
            if (!commandBuffer) {
                return false;
            }

            switch (command.type) {
                case RenderCommandType::BindPipeline:
                    RecordBindPipeline(commandBuffer, command.params.bindPipeline);
                    return true;
                case RenderCommandType::BindDescriptorSets:
                    RecordBindDescriptorSets(commandBuffer, command.params.bindDescriptorSets);
                    return true;
                case RenderCommandType::BindVertexBuffers:
                    RecordBindVertexBuffers(commandBuffer, command.params.bindVertexBuffers);
                    return true;
                case RenderCommandType::BindIndexBuffer:
                    RecordBindIndexBuffer(commandBuffer, command.params.bindIndexBuffer);
                    return true;
                case RenderCommandType::Draw:
                    // Only draw if we're in a valid render pass
                    if (renderPassDepth > 0) {
                        RecordDraw(commandBuffer, command.params.draw);
                        return true;
                    } else {
                        std::cerr << "Error: CommandRecorder: Attempted to call Draw without an active render pass. "
                                  << "Current depth: " << renderPassDepth << std::endl;
                        return false;
                    }
                case RenderCommandType::DrawIndexed:
                    // Only draw if we're in a valid render pass
                    if (renderPassDepth > 0) {
                        RecordDrawIndexed(commandBuffer, command.params.drawIndexed);
                        return true;
                    } else {
                        std::cerr << "Error: CommandRecorder: Attempted to call DrawIndexed without an active render pass. "
                                  << "Current depth: " << renderPassDepth << std::endl;
                        std::cerr << "  This usually means BeginRenderPass failed or was not called. "
                                  << "Check previous error messages for BeginRenderPass failures." << std::endl;
                        return false;
                    }
                case RenderCommandType::TransitionImageLayout:
                    RecordTransitionImageLayout(commandBuffer, command.params.transitionImageLayout);
                    return true;
                case RenderCommandType::BeginRenderPass:
                    // Reset pipeline state when starting a new render pass
                    // This ensures we always bind a pipeline after BeginRenderPass
                    m_CurrentPipeline = nullptr;
                    return RecordBeginRenderPass(commandBuffer, command.params.beginRenderPass);
                case RenderCommandType::EndRenderPass:
                    // Only call EndRenderPass if we're in a valid render pass
                    // renderPassDepth > 0 means we have at least one active BeginRenderPass
                    if (renderPassDepth > 0) {
                        RecordEndRenderPass(commandBuffer, command.params.endRenderPass);
                        return true;
                    } else {
                        std::cerr << "Error: CommandRecorder: Attempted to call EndRenderPass without a matching BeginRenderPass. "
                                  << "Current depth: " << renderPassDepth << std::endl;
                        return false;
                    }
                case RenderCommandType::PushConstants:
                    RecordPushConstants(commandBuffer, command.params.pushConstants);
                    return true;
                default:
                    // Unknown command type, skip
                    return false;
            }
        }

        void CommandRecorder::RecordBindPipeline(RHI::ICommandBuffer* cmd, const RenderCommand::BindPipelineParams& params) {
            if (params.pipeline) {
                cmd->BindPipeline(params.pipeline);
                // Track current pipeline for PushConstants
                m_CurrentPipeline = params.pipeline;
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
            // Ensure pipeline is bound before drawing
            if (!m_CurrentPipeline) {
                std::cerr << "Error: CommandRecorder::RecordDrawIndexed: No pipeline bound. "
                          << "Ensure BindPipeline is called before DrawIndexed." << std::endl;
                return; // Don't draw if no pipeline is bound
            }
            cmd->DrawIndexed(params.indexCount, params.instanceCount, params.firstIndex, params.vertexOffset, params.firstInstance);
        }

        void CommandRecorder::RecordTransitionImageLayout(RHI::ICommandBuffer* cmd, const RenderCommand::TransitionImageLayoutParams& params) {
            if (params.image) {
                // Use accessMode to determine target layout instead of guessing from format
                // This is set by FrameGraph based on AddReadResource/AddWriteResource
                cmd->TransitionImageLayout(params.image, params.formatOld, params.formatNew, params.mipLevels, params.accessMode);
            }
        }

        bool CommandRecorder::RecordBeginRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::BeginRenderPassParams& params) {
            if (!cmd) {
                std::cerr << "Error: CommandRecorder::RecordBeginRenderPass: commandBuffer is nullptr" << std::endl;
                return false;
            }
            
            if (!params.renderPass || !params.framebuffer) {
                std::cerr << "Error: CommandRecorder::RecordBeginRenderPass: Invalid renderPass or framebuffer (nullptr)" << std::endl;
                std::cerr << "  renderPass: " << (params.renderPass ? "valid" : "nullptr") << std::endl;
                std::cerr << "  framebuffer: " << (params.framebuffer ? "valid" : "nullptr") << std::endl;
                return false;
            }
            
            // Check if framebuffer dimensions are valid
            uint32_t width = params.framebuffer->GetWidth();
            uint32_t height = params.framebuffer->GetHeight();
            if (width == 0 || height == 0) {
                std::cerr << "Error: CommandRecorder::RecordBeginRenderPass: Invalid framebuffer dimensions: " 
                          << width << "x" << height << std::endl;
                return false;
            }
            
            try {
                cmd->BeginRenderPass(
                    params.renderPass,
                    params.framebuffer,
                    params.clearColors,
                    params.clearDepth,
                    params.clearStencil
                );
                
                // Set viewport and scissor after beginning render pass
                // This is required for pipelines with dynamic viewport/scissor state
                cmd->SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
                cmd->SetScissor(0, 0, width, height);
                
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error: CommandRecorder::RecordBeginRenderPass: Exception thrown: " << e.what() << std::endl;
                std::cerr << "  renderPass: " << (params.renderPass ? "valid" : "nullptr") << std::endl;
                std::cerr << "  framebuffer: " << (params.framebuffer ? "valid" : "nullptr") << std::endl;
                if (params.framebuffer) {
                    std::cerr << "  framebuffer dimensions: " << width << "x" << height << std::endl;
                }
                return false;
            } catch (...) {
                std::cerr << "Error: CommandRecorder::RecordBeginRenderPass: Unknown exception thrown" << std::endl;
                return false;
            }
        }

        void CommandRecorder::RecordEndRenderPass(RHI::ICommandBuffer* cmd, const RenderCommand::EndRenderPassParams& params) {
            (void)params; // Unused
            // Only call EndRenderPass if we're in a valid render pass state
            // This check is done in RecordCommand before calling this method
            cmd->EndRenderPass();
        }

        void CommandRecorder::RecordPushConstants(RHI::ICommandBuffer* cmd, const RenderCommand::PushConstantsParams& params) {
            if (!cmd || !params.data || params.size == 0) {
                return;
            }

            // Convert stageFlags (uint32_t) to ShaderStage enum
            RHI::ShaderStage stageFlags = static_cast<RHI::ShaderStage>(params.stageFlags);
            
            // Use the currently bound pipeline (tracked in RecordBindPipeline)
            if (m_CurrentPipeline) {
                cmd->PushConstants(m_CurrentPipeline, stageFlags, params.offset, params.size, params.data);
            } else {
                std::cerr << "Warning: CommandRecorder::RecordPushConstants: No pipeline bound. "
                          << "Ensure BindPipeline is called before PushConstants." << std::endl;
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
