#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include <cstring>

namespace FirstEngine {
    namespace Renderer {

        RenderCommand::~RenderCommand() {
            // Clean up any dynamically allocated data in params
            CleanupParams();
        }

        void RenderCommand::CleanupParams() {
            switch (type) {
                case RenderCommandType::BindDescriptorSets:
                    params.bindDescriptorSets.descriptorSets.clear();
                    params.bindDescriptorSets.dynamicOffsets.clear();
                    break;
                case RenderCommandType::BindVertexBuffers:
                    params.bindVertexBuffers.buffers.clear();
                    params.bindVertexBuffers.offsets.clear();
                    break;
                case RenderCommandType::BeginRenderPass:
                    params.beginRenderPass.clearColors.clear();
                    break;
                case RenderCommandType::PushConstants:
                    // Note: data pointer is not owned, just clear the pointer
                    params.pushConstants.data = nullptr;
                    break;
                default:
                    break;
            }
        }

        RenderCommand::RenderCommand(const RenderCommand& other)
            : type(other.type) {
            CopyParams(other);
        }

        RenderCommand& RenderCommand::operator=(const RenderCommand& other) {
            if (this != &other) {
                CleanupParams();
                type = other.type;
                CopyParams(other);
            }
            return *this;
        }

        RenderCommand::RenderCommand(RenderCommand&& other) noexcept
            : type(other.type) {
            MoveParams(std::move(other));
        }

        RenderCommand& RenderCommand::operator=(RenderCommand&& other) noexcept {
            if (this != &other) {
                CleanupParams();
                type = other.type;
                MoveParams(std::move(other));
            }
            return *this;
        }

        void RenderCommand::CopyParams(const RenderCommand& other) {
            switch (type) {
                case RenderCommandType::BindPipeline:
                    params.bindPipeline = other.params.bindPipeline;
                    break;
                case RenderCommandType::BindDescriptorSets:
                    params.bindDescriptorSets.descriptorSets = other.params.bindDescriptorSets.descriptorSets;
                    params.bindDescriptorSets.dynamicOffsets = other.params.bindDescriptorSets.dynamicOffsets;
                    params.bindDescriptorSets.firstSet = other.params.bindDescriptorSets.firstSet;
                    break;
                case RenderCommandType::BindVertexBuffers:
                    params.bindVertexBuffers.buffers = other.params.bindVertexBuffers.buffers;
                    params.bindVertexBuffers.offsets = other.params.bindVertexBuffers.offsets;
                    params.bindVertexBuffers.firstBinding = other.params.bindVertexBuffers.firstBinding;
                    break;
                case RenderCommandType::BindIndexBuffer:
                    params.bindIndexBuffer = other.params.bindIndexBuffer;
                    break;
                case RenderCommandType::Draw:
                    params.draw = other.params.draw;
                    break;
                case RenderCommandType::DrawIndexed:
                    params.drawIndexed = other.params.drawIndexed;
                    break;
                case RenderCommandType::TransitionImageLayout:
                    params.transitionImageLayout = other.params.transitionImageLayout;
                    break;
                case RenderCommandType::BeginRenderPass:
                    params.beginRenderPass.renderPass = other.params.beginRenderPass.renderPass;
                    params.beginRenderPass.framebuffer = other.params.beginRenderPass.framebuffer;
                    params.beginRenderPass.width = other.params.beginRenderPass.width;
                    params.beginRenderPass.height = other.params.beginRenderPass.height;
                    params.beginRenderPass.clearColors = other.params.beginRenderPass.clearColors;
                    params.beginRenderPass.clearDepth = other.params.beginRenderPass.clearDepth;
                    params.beginRenderPass.clearStencil = other.params.beginRenderPass.clearStencil;
                    break;
                case RenderCommandType::EndRenderPass:
                    params.endRenderPass = other.params.endRenderPass;
                    break;
                case RenderCommandType::PushConstants:
                    params.pushConstants = other.params.pushConstants;
                    // Note: data pointer is shallow copied (not owned)
                    break;
                default:
                    break;
            }
        }

        void RenderCommand::MoveParams(RenderCommand&& other) {
            switch (type) {
                case RenderCommandType::BindPipeline:
                    params.bindPipeline = other.params.bindPipeline;
                    break;
                case RenderCommandType::BindDescriptorSets:
                    params.bindDescriptorSets = std::move(other.params.bindDescriptorSets);
                    break;
                case RenderCommandType::BindVertexBuffers:
                    params.bindVertexBuffers = std::move(other.params.bindVertexBuffers);
                    break;
                case RenderCommandType::BindIndexBuffer:
                    params.bindIndexBuffer = other.params.bindIndexBuffer;
                    break;
                case RenderCommandType::Draw:
                    params.draw = other.params.draw;
                    break;
                case RenderCommandType::DrawIndexed:
                    params.drawIndexed = other.params.drawIndexed;
                    break;
                case RenderCommandType::TransitionImageLayout:
                    params.transitionImageLayout = other.params.transitionImageLayout;
                    break;
                case RenderCommandType::BeginRenderPass:
                    params.beginRenderPass.renderPass = other.params.beginRenderPass.renderPass;
                    params.beginRenderPass.framebuffer = other.params.beginRenderPass.framebuffer;
                    params.beginRenderPass.width = other.params.beginRenderPass.width;
                    params.beginRenderPass.height = other.params.beginRenderPass.height;
                    params.beginRenderPass.clearColors = std::move(other.params.beginRenderPass.clearColors);
                    params.beginRenderPass.clearDepth = other.params.beginRenderPass.clearDepth;
                    params.beginRenderPass.clearStencil = other.params.beginRenderPass.clearStencil;
                    break;
                case RenderCommandType::EndRenderPass:
                    params.endRenderPass = other.params.endRenderPass;
                    break;
                case RenderCommandType::PushConstants:
                    params.pushConstants = other.params.pushConstants;
                    other.params.pushConstants.data = nullptr;
                    break;
                default:
                    break;
            }
        }

        RenderCommandList::RenderCommandList() = default;
        RenderCommandList::~RenderCommandList() = default;

        void RenderCommandList::AddCommand(const RenderCommand& command) {
            m_Commands.push_back(command);
        }

        void RenderCommandList::AddCommand(RenderCommand&& command) {
            m_Commands.push_back(std::move(command));
        }

        void RenderCommandList::Clear() {
            m_Commands.clear();
        }

    } // namespace Renderer
} // namespace FirstEngine
