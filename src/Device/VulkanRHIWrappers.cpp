#include "FirstEngine/Device/VulkanRHIWrappers.h"
#include "FirstEngine/Device/RenderPass.h"
#include "FirstEngine/Device/Framebuffer.h"
#include "FirstEngine/Device/Pipeline.h"
#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Device/Swapchain.h"
#include "FirstEngine/Device/ShaderModule.h"
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        // Type conversion function implementations
        VkFormat ConvertFormat(RHI::Format format) {
            switch (format) {
                case RHI::Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
                case RHI::Format::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
                case RHI::Format::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
                case RHI::Format::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
                case RHI::Format::D32_SFLOAT: return VK_FORMAT_D32_SFLOAT;
                case RHI::Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
                default: return VK_FORMAT_UNDEFINED;
            }
        }

        VkBufferUsageFlags ConvertBufferUsage(RHI::BufferUsageFlags usage) {
            VkBufferUsageFlags flags = 0;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::VertexBuffer))
                flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::IndexBuffer))
                flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::UniformBuffer))
                flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::StorageBuffer))
                flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::TransferSrc))
                flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::BufferUsageFlags::TransferDst))
                flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            return flags;
        }

        VkMemoryPropertyFlags ConvertMemoryProperties(RHI::MemoryPropertyFlags properties) {
            VkMemoryPropertyFlags flags = 0;
            if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(RHI::MemoryPropertyFlags::DeviceLocal))
                flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostVisible))
                flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostCoherent))
                flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            if (static_cast<uint32_t>(properties) & static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostCached))
                flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            return flags;
        }

        VkImageUsageFlags ConvertImageUsage(RHI::ImageUsageFlags usage) {
            VkImageUsageFlags flags = 0;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::Sampled))
                flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::Storage))
                flags |= VK_IMAGE_USAGE_STORAGE_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::ColorAttachment))
                flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::DepthStencilAttachment))
                flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::TransferSrc))
                flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            if (static_cast<uint32_t>(usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::TransferDst))
                flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            return flags;
        }

        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage) {
            switch (stage) {
                case RHI::ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
                case RHI::ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
                case RHI::ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
                case RHI::ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
                case RHI::ShaderStage::TessellationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case RHI::ShaderStage::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                default: return VK_SHADER_STAGE_VERTEX_BIT;
            }
        }

        VkPrimitiveTopology ConvertPrimitiveTopology(RHI::PrimitiveTopology topology) {
            switch (topology) {
                case RHI::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case RHI::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                case RHI::PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case RHI::PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            }
        }

        VkCullModeFlags ConvertCullMode(RHI::CullMode cullMode) {
            switch (cullMode) {
                case RHI::CullMode::None: return VK_CULL_MODE_NONE;
                case RHI::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
                case RHI::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
                case RHI::CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
                default: return VK_CULL_MODE_BACK_BIT;
            }
        }

        VkCompareOp ConvertCompareOp(RHI::CompareOp compareOp) {
            switch (compareOp) {
                case RHI::CompareOp::Never: return VK_COMPARE_OP_NEVER;
                case RHI::CompareOp::Less: return VK_COMPARE_OP_LESS;
                case RHI::CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
                case RHI::CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
                case RHI::CompareOp::Greater: return VK_COMPARE_OP_GREATER;
                case RHI::CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
                case RHI::CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case RHI::CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
                default: return VK_COMPARE_OP_LESS;
            }
        }

        VkImageLayout ConvertImageLayout(RHI::Format format) {
            // Temporary implementation: infer layout from format
            // TODO: Should use dedicated Layout enum
            if (format == RHI::Format::Undefined) {
                return VK_IMAGE_LAYOUT_UNDEFINED;
            }
            // Default to general layout
            return VK_IMAGE_LAYOUT_GENERAL;
        }

        // VulkanCommandBuffer implementation
        VulkanCommandBuffer::VulkanCommandBuffer(DeviceContext* context)
            : m_Context(context), m_VkCommandBuffer(VK_NULL_HANDLE), m_IsRecording(false) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = context->GetCommandPool();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            if (vkAllocateCommandBuffers(context->GetDevice(), &allocInfo, &m_VkCommandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate command buffer!");
            }
        }

        VulkanCommandBuffer::~VulkanCommandBuffer() {
            if (m_VkCommandBuffer != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(m_Context->GetDevice(), m_Context->GetCommandPool(), 1, &m_VkCommandBuffer);
            }
        }

        void VulkanCommandBuffer::Begin() {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            if (vkBeginCommandBuffer(m_VkCommandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("Failed to begin recording command buffer!");
            }
            m_IsRecording = true;
        }

        void VulkanCommandBuffer::End() {
            if (vkEndCommandBuffer(m_VkCommandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to end recording command buffer!");
            }
            m_IsRecording = false;
        }

        void VulkanCommandBuffer::BeginRenderPass(RHI::IRenderPass* renderPass, RHI::IFramebuffer* framebuffer,
                                                  const std::vector<float>& clearColors, float clearDepth, uint32_t clearStencil) {
            auto* vkRenderPass = static_cast<VulkanRenderPass*>(renderPass);
            auto* vkFramebuffer = static_cast<VulkanFramebuffer*>(framebuffer);

            std::vector<VkClearValue> clearValues;
            for (size_t i = 0; i < clearColors.size(); i += 4) {
                VkClearValue clearValue{};
                clearValue.color = {clearColors[i], clearColors[i + 1], clearColors[i + 2], clearColors[i + 3]};
                clearValues.push_back(clearValue);
            }
            if (clearValues.empty()) {
                VkClearValue clearValue{};
                clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
                clearValues.push_back(clearValue);
            }

            VkClearValue depthClear{};
            depthClear.depthStencil = {clearDepth, clearStencil};
            clearValues.push_back(depthClear);

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = vkRenderPass->GetVkRenderPass();
            renderPassInfo.framebuffer = vkFramebuffer->GetVkFramebuffer();
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight()};
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(m_VkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        void VulkanCommandBuffer::EndRenderPass() {
            vkCmdEndRenderPass(m_VkCommandBuffer);
        }

        void VulkanCommandBuffer::BindPipeline(RHI::IPipeline* pipeline) {
            auto* vkPipeline = static_cast<VulkanPipeline*>(pipeline);
            vkCmdBindPipeline(m_VkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
        }

        void VulkanCommandBuffer::BindVertexBuffers(uint32_t firstBinding, const std::vector<RHI::IBuffer*>& buffers,
                                                    const std::vector<uint64_t>& offsets) {
            std::vector<VkBuffer> vkBuffers;
            for (auto* buffer : buffers) {
                auto* vkBuffer = static_cast<VulkanBuffer*>(buffer);
                vkBuffers.push_back(vkBuffer->GetVkBuffer());
            }
            vkCmdBindVertexBuffers(m_VkCommandBuffer, firstBinding, static_cast<uint32_t>(vkBuffers.size()),
                                 vkBuffers.data(), offsets.data());
        }

        void VulkanCommandBuffer::BindIndexBuffer(RHI::IBuffer* buffer, uint64_t offset, bool use32BitIndices) {
            auto* vkBuffer = static_cast<VulkanBuffer*>(buffer);
            vkCmdBindIndexBuffer(m_VkCommandBuffer, vkBuffer->GetVkBuffer(), offset,
                                use32BitIndices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
        }

        void VulkanCommandBuffer::BindDescriptorSets(uint32_t firstSet, const std::vector<void*>& descriptorSets,
                                                     const std::vector<uint32_t>& dynamicOffsets) {
            // TODO: Implement descriptor set binding
        }

        void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
            vkCmdDraw(m_VkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
        }

        void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                              int32_t vertexOffset, uint32_t firstInstance) {
            vkCmdDrawIndexed(m_VkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        void VulkanCommandBuffer::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
            VkViewport viewport{};
            viewport.x = x;
            viewport.y = y;
            viewport.width = width;
            viewport.height = height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;
            vkCmdSetViewport(m_VkCommandBuffer, 0, 1, &viewport);
        }

        void VulkanCommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
            VkRect2D scissor{};
            scissor.offset = {x, y};
            scissor.extent = {width, height};
            vkCmdSetScissor(m_VkCommandBuffer, 0, 1, &scissor);
        }

        void VulkanCommandBuffer::TransitionImageLayout(RHI::IImage* image, RHI::Format oldLayout, RHI::Format newLayout, uint32_t mipLevels) {
            // TODO: Implement image layout transition
        }

        void VulkanCommandBuffer::CopyBuffer(RHI::IBuffer* src, RHI::IBuffer* dst, uint64_t size) {
            auto* vkSrc = static_cast<VulkanBuffer*>(src);
            auto* vkDst = static_cast<VulkanBuffer*>(dst);
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(m_VkCommandBuffer, vkSrc->GetVkBuffer(), vkDst->GetVkBuffer(), 1, &copyRegion);
        }

        void VulkanCommandBuffer::CopyBufferToImage(RHI::IBuffer* buffer, RHI::IImage* image, uint32_t width, uint32_t height) {
            auto* vkBuffer = static_cast<VulkanBuffer*>(buffer);
            auto* vkImage = static_cast<VulkanImage*>(image);
            
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, height, 1};

            vkCmdCopyBufferToImage(m_VkCommandBuffer, vkBuffer->GetVkBuffer(), vkImage->GetVkImage(), 
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        // VulkanRenderPass implementation
        VulkanRenderPass::VulkanRenderPass(DeviceContext* context, VkRenderPass renderPass)
            : m_Context(context), m_RenderPass(renderPass) {
        }

        VulkanRenderPass::~VulkanRenderPass() {
            if (m_RenderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(m_Context->GetDevice(), m_RenderPass, nullptr);
            }
        }

        // VulkanFramebuffer implementation
        VulkanFramebuffer::VulkanFramebuffer(DeviceContext* context, VkFramebuffer framebuffer, uint32_t width, uint32_t height)
            : m_Context(context), m_Framebuffer(framebuffer), m_Width(width), m_Height(height) {
        }

        VulkanFramebuffer::~VulkanFramebuffer() {
            if (m_Framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_Context->GetDevice(), m_Framebuffer, nullptr);
            }
        }

        // VulkanPipeline implementation
        VulkanPipeline::VulkanPipeline(DeviceContext* context, VkPipeline pipeline, VkPipelineLayout layout)
            : m_Context(context), m_Pipeline(pipeline), m_PipelineLayout(layout) {
        }

        VulkanPipeline::~VulkanPipeline() {
            if (m_Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Context->GetDevice(), m_Pipeline, nullptr);
            }
            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Context->GetDevice(), m_PipelineLayout, nullptr);
            }
        }

        // VulkanBuffer implementation
        VulkanBuffer::VulkanBuffer(DeviceContext* context, Buffer* buffer)
            : m_Context(context), m_Buffer(buffer) {
        }

        VulkanBuffer::~VulkanBuffer() = default;

        uint64_t VulkanBuffer::GetSize() const {
            return m_Buffer ? m_Buffer->GetSize() : 0;
        }

        void* VulkanBuffer::Map() {
            return m_Buffer ? m_Buffer->Map() : nullptr;
        }

        void VulkanBuffer::Unmap() {
            if (m_Buffer) {
                m_Buffer->Unmap();
            }
        }

        void VulkanBuffer::UpdateData(const void* data, uint64_t size, uint64_t offset) {
            if (m_Buffer) {
                m_Buffer->UpdateData(data, size, offset);
            }
        }

        VkBuffer VulkanBuffer::GetVkBuffer() const {
            return m_Buffer ? m_Buffer->GetBuffer() : VK_NULL_HANDLE;
        }

        // VulkanImageView implementation
        VulkanImageView::VulkanImageView(DeviceContext* context, VkImageView imageView)
            : m_Context(context), m_ImageView(imageView) {
        }

        VulkanImageView::~VulkanImageView() {
            if (m_ImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Context->GetDevice(), m_ImageView, nullptr);
            }
        }

        // VulkanImage implementation
        VulkanImage::VulkanImage(DeviceContext* context, Image* image)
            : m_Context(context), m_Image(image) {
        }

        VulkanImage::~VulkanImage() = default;

        uint32_t VulkanImage::GetWidth() const {
            return m_Image ? m_Image->GetWidth() : 0;
        }

        uint32_t VulkanImage::GetHeight() const {
            return m_Image ? m_Image->GetHeight() : 0;
        }

        RHI::Format VulkanImage::GetFormat() const {
            if (!m_Image) {
                return RHI::Format::Undefined;
            }
            
            VkFormat vkFormat = m_Image->GetFormat();
            // Convert VkFormat to RHI::Format
            switch (vkFormat) {
                case VK_FORMAT_B8G8R8A8_UNORM: return RHI::Format::B8G8R8A8_UNORM;
                case VK_FORMAT_B8G8R8A8_SRGB: return RHI::Format::B8G8R8A8_SRGB;
                case VK_FORMAT_R8G8B8A8_UNORM: return RHI::Format::R8G8B8A8_UNORM;
                case VK_FORMAT_R8G8B8A8_SRGB: return RHI::Format::R8G8B8A8_SRGB;
                case VK_FORMAT_D32_SFLOAT: return RHI::Format::D32_SFLOAT;
                case VK_FORMAT_D24_UNORM_S8_UINT: return RHI::Format::D24_UNORM_S8_UINT;
                default: return RHI::Format::Undefined;
            }
        }

        RHI::IImageView* VulkanImage::CreateImageView() {
            if (!m_Image) return nullptr;
            VkImageView imageView = m_Image->GetImageView();
            if (imageView == VK_NULL_HANDLE) {
                // Create image view
                // TODO: Implement image view creation
                return nullptr;
            }
            auto view = std::make_unique<VulkanImageView>(m_Context, imageView);
            auto* viewPtr = view.get();
            m_ImageViews.push_back(std::move(view));
            return viewPtr;
        }

        void VulkanImage::DestroyImageView(RHI::IImageView* imageView) {
            // Remove from m_ImageViews
            for (auto it = m_ImageViews.begin(); it != m_ImageViews.end(); ++it) {
                if (it->get() == imageView) {
                    m_ImageViews.erase(it);
                    break;
                }
            }
        }

        VkImage VulkanImage::GetVkImage() const {
            return m_Image ? m_Image->GetImage() : VK_NULL_HANDLE;
        }

        // VulkanSwapchain implementation
        VulkanSwapchain::VulkanSwapchain(DeviceContext* context, Swapchain* swapchain)
            : m_Context(context), m_Swapchain(swapchain) {
            // Get swapchain images
            if (m_Swapchain) {
                auto images = m_Swapchain->GetImages();
                auto imageViews = m_Swapchain->GetImageViews();
                // Note: Swapchain images are VkImage, not our Image class
                // We need to create wrappers for each swapchain image
                // Temporarily not creating, will create dynamically in GetImage
            }
        }

        VulkanSwapchain::~VulkanSwapchain() = default;

        bool VulkanSwapchain::AcquireNextImage(RHI::SemaphoreHandle semaphore, RHI::FenceHandle fence, uint32_t& imageIndex) {
            return m_Swapchain->AcquireNextImage(
                static_cast<VkSemaphore>(semaphore),
                static_cast<VkFence>(fence),
                imageIndex
            );
        }

        bool VulkanSwapchain::Present(uint32_t imageIndex, const std::vector<RHI::SemaphoreHandle>& waitSemaphores) {
            if (!m_Swapchain) {
                return false;
            }
            
            VkSemaphore waitSemaphore = VK_NULL_HANDLE;
            if (!waitSemaphores.empty()) {
                waitSemaphore = static_cast<VkSemaphore>(waitSemaphores[0]);
            }
            
            return m_Swapchain->Present(imageIndex, waitSemaphore);
        }

        uint32_t VulkanSwapchain::GetImageCount() const {
            return m_Swapchain ? m_Swapchain->GetImageCount() : 0;
        }

        RHI::Format VulkanSwapchain::GetImageFormat() const {
            if (!m_Swapchain) {
                return RHI::Format::Undefined;
            }
            
            VkFormat vkFormat = m_Swapchain->GetImageFormat();
            // Convert VkFormat to RHI::Format
            switch (vkFormat) {
                case VK_FORMAT_B8G8R8A8_UNORM: return RHI::Format::B8G8R8A8_UNORM;
                case VK_FORMAT_B8G8R8A8_SRGB: return RHI::Format::B8G8R8A8_SRGB;
                case VK_FORMAT_R8G8B8A8_UNORM: return RHI::Format::R8G8B8A8_UNORM;
                case VK_FORMAT_R8G8B8A8_SRGB: return RHI::Format::R8G8B8A8_SRGB;
                default: return RHI::Format::Undefined;
            }
        }

        void VulkanSwapchain::GetExtent(uint32_t& width, uint32_t& height) const {
            if (m_Swapchain) {
                auto extent = m_Swapchain->GetExtent();
                width = extent.width;
                height = extent.height;
            }
        }

        RHI::IImage* VulkanSwapchain::GetImage(uint32_t index) {
            if (!m_Swapchain || index >= m_Swapchain->GetImageCount()) {
                return nullptr;
            }
            
            // Ensure enough image wrappers
            while (m_Images.size() <= index) {
                m_Images.push_back(nullptr);
            }
            
            // If not created yet, create a temporary wrapper
            // Note: Swapchain images are VkImage, not complete Image objects
            // We need a special handling approach here
            // Temporarily return nullptr, implement when needed
            return nullptr;
        }

        // VulkanShaderModule implementation
        VulkanShaderModule::VulkanShaderModule(DeviceContext* context, ShaderModule* shaderModule, RHI::ShaderStage stage)
            : m_Context(context), m_ShaderModule(shaderModule), m_Stage(stage) {
        }

        VulkanShaderModule::~VulkanShaderModule() = default;

        VkShaderModule VulkanShaderModule::GetVkShaderModule() const {
            return m_ShaderModule ? m_ShaderModule->GetShaderModule() : VK_NULL_HANDLE;
        }

    } // namespace Device
} // namespace FirstEngine
