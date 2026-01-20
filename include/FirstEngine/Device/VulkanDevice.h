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

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/Device/VulkanRenderer.h"
#include <memory>

namespace FirstEngine {
    namespace Device {

        // Vulkan implementation of IDevice
        class FE_DEVICE_API VulkanDevice : public RHI::IDevice {
        public:
            VulkanDevice();
            ~VulkanDevice() override;

            // IDevice interface implementation
            bool Initialize(void* windowHandle) override;
            void Shutdown() override;

            std::unique_ptr<RHI::ICommandBuffer> CreateCommandBuffer() override;
            std::unique_ptr<RHI::IRenderPass> CreateRenderPass(const RHI::RenderPassDescription& desc) override;
            std::unique_ptr<RHI::IFramebuffer> CreateFramebuffer(
                RHI::IRenderPass* renderPass,
                const std::vector<RHI::IImageView*>& attachments,
                uint32_t width, uint32_t height) override;
            std::unique_ptr<RHI::IPipeline> CreateGraphicsPipeline(
                const RHI::GraphicsPipelineDescription& desc) override;
            std::unique_ptr<RHI::IPipeline> CreateComputePipeline(
                const RHI::ComputePipelineDescription& desc) override;
            std::unique_ptr<RHI::IBuffer> CreateBuffer(
                uint64_t size, RHI::BufferUsageFlags usage, RHI::MemoryPropertyFlags properties) override;
            std::unique_ptr<RHI::IImage> CreateImage(const RHI::ImageDescription& desc) override;
            std::unique_ptr<RHI::ISwapchain> CreateSwapchain(
                void* windowHandle, const RHI::SwapchainDescription& desc) override;
            std::unique_ptr<RHI::IShaderModule> CreateShaderModule(
                const std::vector<uint32_t>& spirvCode, RHI::ShaderStage stage) override;
#ifdef _WIN32
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif
            RHI::SemaphoreHandle CreateSemaphore() override;
            void DestroySemaphore(RHI::SemaphoreHandle semaphore) override;
            RHI::FenceHandle CreateFence(bool signaled = false) override;
            void DestroyFence(RHI::FenceHandle fence) override;

            void SubmitCommandBuffer(
                RHI::ICommandBuffer* commandBuffer,
                const std::vector<RHI::SemaphoreHandle>& waitSemaphores = {},
                const std::vector<RHI::SemaphoreHandle>& signalSemaphores = {},
                RHI::FenceHandle fence = nullptr) override;

            void WaitIdle() override;

            void WaitForFence(RHI::FenceHandle fence, uint64_t timeout = UINT64_MAX) override;
            void ResetFence(RHI::FenceHandle fence) override;

            RHI::QueueHandle GetGraphicsQueue() const override;
            RHI::QueueHandle GetPresentQueue() const override;

            const RHI::DeviceInfo& GetDeviceInfo() const override;

            // Get underlying VulkanRenderer (for advanced operations)
            VulkanRenderer* GetVulkanRenderer() const { return m_Renderer.get(); }

            // Descriptor set operations
            RHI::DescriptorSetLayoutHandle CreateDescriptorSetLayout(
                const RHI::DescriptorSetLayoutDescription& desc) override;
            void DestroyDescriptorSetLayout(RHI::DescriptorSetLayoutHandle layout) override;

            RHI::DescriptorPoolHandle CreateDescriptorPool(
                uint32_t maxSets,
                const std::vector<std::pair<RHI::DescriptorType, uint32_t>>& poolSizes) override;
            void DestroyDescriptorPool(RHI::DescriptorPoolHandle pool) override;

            std::vector<RHI::DescriptorSetHandle> AllocateDescriptorSets(
                RHI::DescriptorPoolHandle pool,
                const std::vector<RHI::DescriptorSetLayoutHandle>& layouts) override;
            void FreeDescriptorSets(
                RHI::DescriptorPoolHandle pool,
                const std::vector<RHI::DescriptorSetHandle>& sets) override;

            void UpdateDescriptorSets(
                const std::vector<RHI::DescriptorWrite>& writes) override;

        private:
            std::unique_ptr<VulkanRenderer> m_Renderer;
            std::unique_ptr<Core::Window> m_Window;
            RHI::DeviceInfo m_DeviceInfo;
        };

    } // namespace Device
} // namespace FirstEngine

#ifdef _WIN32
// Restore Windows API macros
#pragma pop_macro("CreateEvent")
#pragma pop_macro("CreateMutex")
#pragma pop_macro("CreateSemaphore")
#endif
