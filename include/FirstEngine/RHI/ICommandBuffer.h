#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"
#include <vector>

namespace FirstEngine {
    namespace RHI {

        // Forward declarations
        class IRenderPass;
        class IFramebuffer;
        class IPipeline;
        class IBuffer;
        class IImage;

        // Command buffer interface
        class FE_RHI_API ICommandBuffer {
        public:
            virtual ~ICommandBuffer() = default;

            // Record commands
            virtual void Begin() = 0;
            virtual void End() = 0;

            // Render pass
            virtual void BeginRenderPass(
                IRenderPass* renderPass,
                IFramebuffer* framebuffer,
                const std::vector<float>& clearColors,
                float clearDepth = 1.0f,
                uint32_t clearStencil = 0) = 0;
            virtual void EndRenderPass() = 0;

            // Pipeline binding
            virtual void BindPipeline(IPipeline* pipeline) = 0;

            // Vertex buffer binding
            virtual void BindVertexBuffers(
                uint32_t firstBinding,
                const std::vector<IBuffer*>& buffers,
                const std::vector<uint64_t>& offsets) = 0;
            virtual void BindIndexBuffer(IBuffer* buffer, uint64_t offset, bool use32BitIndices = false) = 0;

            // Descriptor set binding (temporarily using void*, will be abstracted later)
            virtual void BindDescriptorSets(
                uint32_t firstSet,
                const std::vector<void*>& descriptorSets,
                const std::vector<uint32_t>& dynamicOffsets = {}) = 0;

            // Draw commands
            virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                            uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
            virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                                    uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                    uint32_t firstInstance = 0) = 0;

            // Viewport and scissor
            virtual void SetViewport(float x, float y, float width, float height,
                                    float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
            virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

            // Image layout transition
            // accessMode: Read -> SHADER_READ_ONLY_OPTIMAL, Write -> COLOR_ATTACHMENT_OPTIMAL or DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            virtual void TransitionImageLayout(
                IImage* image,
                Format oldLayout,
                Format newLayout,
                uint32_t mipLevels = 1,
                ImageAccessMode accessMode = ImageAccessMode::Read) = 0;

            // Buffer copy
            virtual void CopyBuffer(IBuffer* src, IBuffer* dst, uint64_t size) = 0;
            virtual void CopyBufferToImage(IBuffer* buffer, IImage* image, uint32_t width, uint32_t height) = 0;

            // Push constants
            virtual void PushConstants(
                IPipeline* pipeline,
                ShaderStage stageFlags,
                uint32_t offset,
                uint32_t size,
                const void* data) = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
