#pragma once

#ifdef _WIN32
// Push and undefine Windows API macros that conflict with our method names
#pragma push_macro("CreateSemaphore")
#pragma push_macro("CreateMutex")
#pragma push_macro("CreateEvent")
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif
#ifdef CreateMutex
#undef CreateMutex
#endif
#ifdef CreateEvent
#undef CreateEvent
#endif
#endif

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IImage.h"  // Need complete definition of IImageView
#include <memory>
#include <vector>
#include <string>

namespace FirstEngine {
    namespace RHI {

        // Forward declarations
        class ICommandBuffer;
        class IRenderPass;
        class IFramebuffer;
        class IPipeline;
        class IBuffer;
        class IImage;
        class ISwapchain;
        class IShaderModule;

        // Device interface - abstraction for all graphics APIs
        class FE_RHI_API IDevice {
        public:
            virtual ~IDevice() = default;

            // Device initialization
            virtual bool Initialize(void* windowHandle) = 0;
            virtual void Shutdown() = 0;

            // Command buffer creation
            virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer() = 0;

            // Render pass creation
            virtual std::unique_ptr<IRenderPass> CreateRenderPass(const RenderPassDescription& desc) = 0;

            // Framebuffer creation
            virtual std::unique_ptr<IFramebuffer> CreateFramebuffer(
                IRenderPass* renderPass,
                const std::vector<IImageView*>& attachments,
                uint32_t width, uint32_t height) = 0;

            // Pipeline creation
            virtual std::unique_ptr<IPipeline> CreateGraphicsPipeline(
                const GraphicsPipelineDescription& desc) = 0;
            virtual std::unique_ptr<IPipeline> CreateComputePipeline(
                const ComputePipelineDescription& desc) = 0;

            // Buffer creation
            virtual std::unique_ptr<IBuffer> CreateBuffer(
                uint64_t size, BufferUsageFlags usage, MemoryPropertyFlags properties) = 0;

            // Image creation
            virtual std::unique_ptr<IImage> CreateImage(
                const ImageDescription& desc) = 0;

            // Swapchain creation
            virtual std::unique_ptr<ISwapchain> CreateSwapchain(
                void* windowHandle, const SwapchainDescription& desc) = 0;

            // Shader module creation
            virtual std::unique_ptr<IShaderModule> CreateShaderModule(
                const std::vector<uint32_t>& spirvCode, ShaderStage stage) = 0;

            // Synchronization object creation
            virtual SemaphoreHandle CreateSemaphore() = 0;
            virtual void DestroySemaphore(SemaphoreHandle semaphore) = 0;
            virtual FenceHandle CreateFence(bool signaled = false) = 0;
            virtual void DestroyFence(FenceHandle fence) = 0;

            // Queue operations
            virtual void SubmitCommandBuffer(
                ICommandBuffer* commandBuffer,
                const std::vector<SemaphoreHandle>& waitSemaphores = {},
                const std::vector<SemaphoreHandle>& signalSemaphores = {},
                FenceHandle fence = nullptr) = 0;

            virtual void WaitIdle() = 0;

            // Fence operations
            virtual void WaitForFence(FenceHandle fence, uint64_t timeout = UINT64_MAX) = 0;
            virtual void ResetFence(FenceHandle fence) = 0;

            // Get queues
            virtual QueueHandle GetGraphicsQueue() const = 0;
            virtual QueueHandle GetPresentQueue() const = 0;

            // Get device information
            virtual const DeviceInfo& GetDeviceInfo() const = 0;
        };

    } // namespace RHI
} // namespace FirstEngine

#ifdef _WIN32
// Restore Windows API macros
#pragma pop_macro("CreateEvent")
#pragma pop_macro("CreateMutex")
#pragma pop_macro("CreateSemaphore")
#endif
