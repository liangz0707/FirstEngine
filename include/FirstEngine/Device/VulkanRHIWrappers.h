#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/ICommandBuffer.h"  // Need complete definition of ICommandBuffer
#include "FirstEngine/RHI/IRenderPass.h"      // Need complete definition of IRenderPass
#include "FirstEngine/RHI/IFramebuffer.h"     // Need complete definition of IFramebuffer
#include "FirstEngine/RHI/IPipeline.h"         // Need complete definition of IPipeline
#include "FirstEngine/RHI/IBuffer.h"          // Need complete definition of IBuffer
#include "FirstEngine/RHI/IImage.h"            // Need complete definition of IImage and IImageView
#include "FirstEngine/RHI/ISwapchain.h"        // Need complete definition of ISwapchain
#include "FirstEngine/RHI/IShaderModule.h"    // Need complete definition of IShaderModule
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Device {

        // Type conversion functions
        VkFormat ConvertFormat(RHI::Format format);
        VkBufferUsageFlags ConvertBufferUsage(RHI::BufferUsageFlags usage);
        VkMemoryPropertyFlags ConvertMemoryProperties(RHI::MemoryPropertyFlags properties);
        VkImageUsageFlags ConvertImageUsage(RHI::ImageUsageFlags usage);
        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage);
        VkPrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology);
        VkCullModeFlags ConvertCullMode(RHI::CullMode cullMode);
        VkCompareOp ConvertCompareOp(RHI::CompareOp compareOp);
        VkImageLayout ConvertImageLayout(RHI::Format format); // Temporary: should use dedicated Layout enum later

        // Vulkan implementation of RHI interface wrapper classes
        class FE_DEVICE_API VulkanCommandBuffer : public RHI::ICommandBuffer {
        public:
            VulkanCommandBuffer(DeviceContext* context);
            ~VulkanCommandBuffer() override;

            void Begin() override;
            void End() override;
            void BeginRenderPass(RHI::IRenderPass* renderPass, RHI::IFramebuffer* framebuffer,
                               const std::vector<float>& clearColors, float clearDepth, uint32_t clearStencil) override;
            void EndRenderPass() override;
            void BindPipeline(RHI::IPipeline* pipeline) override;
            void BindVertexBuffers(uint32_t firstBinding, const std::vector<RHI::IBuffer*>& buffers,
                                  const std::vector<uint64_t>& offsets) override;
            void BindIndexBuffer(RHI::IBuffer* buffer, uint64_t offset, bool use32BitIndices) override;
            void BindDescriptorSets(uint32_t firstSet, const std::vector<void*>& descriptorSets,
                                   const std::vector<uint32_t>& dynamicOffsets) override;
            void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
            void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                           int32_t vertexOffset, uint32_t firstInstance) override;
            void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
            void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
            void TransitionImageLayout(RHI::IImage* image, RHI::Format oldLayout, RHI::Format newLayout, uint32_t mipLevels) override;
            void CopyBuffer(RHI::IBuffer* src, RHI::IBuffer* dst, uint64_t size) override;
            void CopyBufferToImage(RHI::IBuffer* buffer, RHI::IImage* image, uint32_t width, uint32_t height) override;

            VkCommandBuffer GetVkCommandBuffer() const { return m_VkCommandBuffer; }

        private:
            DeviceContext* m_Context;
            VkCommandBuffer m_VkCommandBuffer;
            bool m_IsRecording;
        };

        class FE_DEVICE_API VulkanRenderPass : public RHI::IRenderPass {
        public:
            VulkanRenderPass(DeviceContext* context, VkRenderPass renderPass);
            ~VulkanRenderPass() override;

            VkRenderPass GetVkRenderPass() const { return m_RenderPass; }

        private:
            DeviceContext* m_Context;
            VkRenderPass m_RenderPass;
        };

        class FE_DEVICE_API VulkanFramebuffer : public RHI::IFramebuffer {
        public:
            VulkanFramebuffer(DeviceContext* context, VkFramebuffer framebuffer, uint32_t width, uint32_t height);
            ~VulkanFramebuffer() override;

            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }

            VkFramebuffer GetVkFramebuffer() const { return m_Framebuffer; }

        private:
            DeviceContext* m_Context;
            VkFramebuffer m_Framebuffer;
            uint32_t m_Width;
            uint32_t m_Height;
        };

        class FE_DEVICE_API VulkanPipeline : public RHI::IPipeline {
        public:
            VulkanPipeline(DeviceContext* context, VkPipeline pipeline, VkPipelineLayout layout);
            ~VulkanPipeline() override;

            VkPipeline GetVkPipeline() const { return m_Pipeline; }
            VkPipelineLayout GetVkPipelineLayout() const { return m_PipelineLayout; }

        private:
            DeviceContext* m_Context;
            VkPipeline m_Pipeline;
            VkPipelineLayout m_PipelineLayout;
        };

        class FE_DEVICE_API VulkanBuffer : public RHI::IBuffer {
        public:
            VulkanBuffer(DeviceContext* context, class Buffer* buffer);
            ~VulkanBuffer() override;

            uint64_t GetSize() const override;
            void* Map() override;
            void Unmap() override;
            void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) override;

            VkBuffer GetVkBuffer() const;

        private:
            DeviceContext* m_Context;
            std::unique_ptr<class Buffer> m_Buffer;
        };

        class FE_DEVICE_API VulkanImageView : public RHI::IImageView {
        public:
            VulkanImageView(DeviceContext* context, VkImageView imageView);
            ~VulkanImageView() override;

            VkImageView GetVkImageView() const { return m_ImageView; }

        private:
            DeviceContext* m_Context;
            VkImageView m_ImageView;
        };

        class FE_DEVICE_API VulkanImage : public RHI::IImage {
        public:
            VulkanImage(DeviceContext* context, class Image* image);
            ~VulkanImage() override;

            uint32_t GetWidth() const override;
            uint32_t GetHeight() const override;
            RHI::Format GetFormat() const override;
            RHI::IImageView* CreateImageView() override;
            void DestroyImageView(RHI::IImageView* imageView) override;

            VkImage GetVkImage() const;

        private:
            DeviceContext* m_Context;
            std::unique_ptr<class Image> m_Image;
            std::vector<std::unique_ptr<VulkanImageView>> m_ImageViews;
        };

        class FE_DEVICE_API VulkanSwapchain : public RHI::ISwapchain {
        public:
            VulkanSwapchain(DeviceContext* context, class Swapchain* swapchain);
            ~VulkanSwapchain() override;

            // Disable copy constructor and copy assignment operator
            VulkanSwapchain(const VulkanSwapchain&) = delete;
            VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

            bool AcquireNextImage(RHI::SemaphoreHandle semaphore, RHI::FenceHandle fence, uint32_t& imageIndex) override;
            bool Present(uint32_t imageIndex, const std::vector<RHI::SemaphoreHandle>& waitSemaphores) override;
            uint32_t GetImageCount() const override;
            RHI::Format GetImageFormat() const override;
            void GetExtent(uint32_t& width, uint32_t& height) const override;
            RHI::IImage* GetImage(uint32_t index) override;

        private:
            DeviceContext* m_Context;
            class Swapchain* m_Swapchain; // Raw pointer, owned by VulkanRenderer
            std::vector<std::unique_ptr<VulkanImage>> m_Images;
        };

        class FE_DEVICE_API VulkanShaderModule : public RHI::IShaderModule {
        public:
            VulkanShaderModule(DeviceContext* context, class ShaderModule* shaderModule, RHI::ShaderStage stage);
            ~VulkanShaderModule() override;

            RHI::ShaderStage GetStage() const override { return m_Stage; }

            VkShaderModule GetVkShaderModule() const;

        private:
            DeviceContext* m_Context;
            std::unique_ptr<class ShaderModule> m_ShaderModule;
            RHI::ShaderStage m_Stage;
        };

    } // namespace Device
} // namespace FirstEngine
