#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/Types.h"
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

// Forward declarations
namespace FirstEngine {
    namespace RHI {
        class IPipeline;
        class IBuffer;
        class IImage;
        class IRenderPass;
        class IFramebuffer;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Render command types - represents all possible GPU commands
        enum class RenderCommandType {
            BindPipeline,
            BindDescriptorSets,
            BindVertexBuffers,
            BindIndexBuffer,
            Draw,
            DrawIndexed,
            DrawIndirect,
            DrawIndexedIndirect,
            TransitionImageLayout,
            BeginRenderPass,
            EndRenderPass,
            CopyBuffer,
            CopyImage,
            BlitImage,
            ClearColorImage,
            ClearDepthStencilImage,
            Dispatch,
            DispatchIndirect,
            PipelineBarrier,
            PushConstants,
        };

        // Render command - a single GPU command instruction
        // This is a data structure that represents a command, not the execution
        class FE_RENDERER_API RenderCommand {
        public:
            RenderCommandType type;

            // Command parameters (union-like structure for different command types)
            struct BindPipelineParams {
                RHI::IPipeline* pipeline;
            };

            struct BindDescriptorSetsParams {
                uint32_t firstSet;
                std::vector<void*> descriptorSets; // Descriptor set handles
                std::vector<uint32_t> dynamicOffsets;
            };

            struct BindVertexBuffersParams {
                uint32_t firstBinding;
                std::vector<RHI::IBuffer*> buffers;
                std::vector<uint64_t> offsets;
            };

            struct BindIndexBufferParams {
                RHI::IBuffer* buffer;
                uint64_t offset;
                bool is32Bit;
            };

            struct DrawParams {
                uint32_t vertexCount;
                uint32_t instanceCount;
                uint32_t firstVertex;
                uint32_t firstInstance;
            };

            struct DrawIndexedParams {
                uint32_t indexCount;
                uint32_t instanceCount;
                uint32_t firstIndex;
                int32_t vertexOffset;
                uint32_t firstInstance;
            };

            struct TransitionImageLayoutParams {
                RHI::IImage* image;
                RHI::Format formatOld;
                RHI::Format formatNew;
                uint32_t mipLevels;
                RHI::ImageAccessMode accessMode; // Read or Write - determines target layout
            };

            struct BeginRenderPassParams {
                RHI::IRenderPass* renderPass;
                RHI::IFramebuffer* framebuffer;
                uint32_t width;
                uint32_t height;
                std::vector<float> clearColors; // RGBA values
                float clearDepth;
                uint32_t clearStencil;
            };

            struct EndRenderPassParams {
                // No parameters needed
            };

            struct PushConstantsParams {
                void* pipelineLayout;
                uint32_t stageFlags;
                uint32_t offset;
                uint32_t size;
                const void* data;
            };

            // Storage for command parameters (using struct instead of union to support std::vector)
            // Only the relevant field should be used based on 'type'
            struct {
                BindPipelineParams bindPipeline;
                BindDescriptorSetsParams bindDescriptorSets;
                BindVertexBuffersParams bindVertexBuffers;
                BindIndexBufferParams bindIndexBuffer;
                DrawParams draw;
                DrawIndexedParams drawIndexed;
                TransitionImageLayoutParams transitionImageLayout;
                BeginRenderPassParams beginRenderPass;
                EndRenderPassParams endRenderPass;
                PushConstantsParams pushConstants;
            } params;

            RenderCommand() = default;
            ~RenderCommand();

            // Copy constructor and assignment (deep copy for vectors)
            RenderCommand(const RenderCommand& other);
            RenderCommand& operator=(const RenderCommand& other);
            RenderCommand(RenderCommand&& other) noexcept;
            RenderCommand& operator=(RenderCommand&& other) noexcept;

        private:
            void CleanupParams();
            void CopyParams(const RenderCommand& other);
            void MoveParams(RenderCommand&& other);
        };

        // Render command list - a list of render commands that can be recorded to CommandBuffer
        class FE_RENDERER_API RenderCommandList {
        public:
            RenderCommandList();
            ~RenderCommandList();

            // Add commands
            void AddCommand(const RenderCommand& command);
            void AddCommand(RenderCommand&& command);

            // Get commands
            const std::vector<RenderCommand>& GetCommands() const { return m_Commands; }
            std::vector<RenderCommand>& GetCommands() { return m_Commands; }

            // Clear all commands
            void Clear();

            // Get command count
            size_t GetCommandCount() const { return m_Commands.size(); }

            // Check if empty
            bool IsEmpty() const { return m_Commands.empty(); }

        private:
            std::vector<RenderCommand> m_Commands;
        };

    } // namespace Renderer
} // namespace FirstEngine
