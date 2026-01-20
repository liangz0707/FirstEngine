#include "FirstEngine/Device/VulkanRHIWrappers.h"
#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Device/ShaderModule.h"  // Still needed for VulkanShaderModule wrapper
#include <algorithm>
#include <stdexcept>
#include <iostream>

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

        VkShaderStageFlags ConvertShaderStageFlags(RHI::ShaderStage stage) {
            VkShaderStageFlags flags = 0;
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::Vertex)) {
                flags |= VK_SHADER_STAGE_VERTEX_BIT;
            }
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::Fragment)) {
                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::Geometry)) {
                flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
            }
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::Compute)) {
                flags |= VK_SHADER_STAGE_COMPUTE_BIT;
            }
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::TessellationControl)) {
                flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            }
            if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(RHI::ShaderStage::TessellationEvaluation)) {
                flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            }
            return flags;
        }

        VkDescriptorType ConvertDescriptorType(RHI::DescriptorType type) {
            switch (type) {
                case RHI::DescriptorType::UniformBuffer:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case RHI::DescriptorType::CombinedImageSampler:
                    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case RHI::DescriptorType::SampledImage:
                    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case RHI::DescriptorType::StorageImage:
                    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case RHI::DescriptorType::StorageBuffer:
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                default:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
        }

        // VulkanCommandBuffer implementation
        VulkanCommandBuffer::VulkanCommandBuffer(DeviceContext* context)
            : m_Context(context), m_VkCommandBuffer(VK_NULL_HANDLE), m_IsRecording(false), m_CurrentPipelineLayout(VK_NULL_HANDLE) {
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
                // Note: Command buffers should only be freed after GPU has finished using them
                // This is typically handled by waiting for a fence before destroying the command buffer
                // For now, we assume the caller has ensured GPU completion (via WaitForFence or WaitIdle)
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
            if (!pipeline) {
                return;
            }
            auto* vkPipeline = static_cast<VulkanPipeline*>(pipeline);
            vkCmdBindPipeline(m_VkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetVkPipeline());
            // Store pipeline layout for descriptor set binding
            m_CurrentPipelineLayout = vkPipeline->GetVkPipelineLayout();
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
            if (descriptorSets.empty() || m_CurrentPipelineLayout == VK_NULL_HANDLE) {
                return;
            }

            // Convert void* descriptor sets to VkDescriptorSet
            std::vector<VkDescriptorSet> vkSets;
            vkSets.reserve(descriptorSets.size());
            for (void* set : descriptorSets) {
                if (set) {
                    vkSets.push_back(reinterpret_cast<VkDescriptorSet>(set));
                }
            }

            if (vkSets.empty()) {
                return;
            }

            // Convert dynamic offsets
            std::vector<uint32_t> vkOffsets = dynamicOffsets;

            // Bind descriptor sets
            vkCmdBindDescriptorSets(
                m_VkCommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_CurrentPipelineLayout,
                firstSet,
                static_cast<uint32_t>(vkSets.size()),
                vkSets.data(),
                static_cast<uint32_t>(vkOffsets.size()),
                vkOffsets.empty() ? nullptr : vkOffsets.data()
            );
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
            if (!image) return;

            auto* vkImage = static_cast<VulkanImage*>(image);
            VkImage vkImg = vkImage->GetVkImage();
            if (vkImg == VK_NULL_HANDLE) return;

            // Use the actual current layout from the image (this is the correct way)
            // The oldLayout parameter is ignored - we use the tracked layout instead
            VkImageLayout oldVkLayout = vkImage->GetCurrentLayout();
            VkImageLayout newVkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Determine new layout based on newLayout Format parameter
            // This is a temporary solution until we have a dedicated Layout enum
            if (newLayout == RHI::Format::B8G8R8A8_SRGB || newLayout == RHI::Format::R8G8B8A8_SRGB) {
                // SRGB format typically means present source for swapchain
                newVkLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            } else if (newLayout == RHI::Format::B8G8R8A8_UNORM || newLayout == RHI::Format::R8G8B8A8_UNORM) {
                // UNORM format: could be COLOR_ATTACHMENT or TRANSFER_DST
                // If current layout is UNDEFINED, assume TRANSFER_DST (texture upload)
                // Otherwise, assume COLOR_ATTACHMENT
                if (oldVkLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                    newVkLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                } else if (oldVkLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    // Already in TRANSFER_DST, next step is SHADER_READ_ONLY
                    newVkLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                } else {
                    // Default to COLOR_ATTACHMENT for render targets
                    newVkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            } else if (newLayout == RHI::Format::D32_SFLOAT || newLayout == RHI::Format::D24_UNORM_S8_UINT) {
                // Depth formats
                if (oldVkLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                    newVkLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                } else {
                    newVkLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
            } else {
                // Default to COLOR_ATTACHMENT
                newVkLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
            
            // If old and new layouts are the same, no transition needed
            if (oldVkLayout == newVkLayout) {
                return;
            }

            // Determine image aspect mask based on format
            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            if (newLayout == RHI::Format::D32_SFLOAT) {
                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            } else if (newLayout == RHI::Format::D24_UNORM_S8_UINT) {
                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldVkLayout;
            barrier.newLayout = newVkLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = vkImg;
            barrier.subresourceRange.aspectMask = aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            // Determine source and destination access masks based on layout transition
            VkAccessFlags srcAccessMask = 0;
            VkAccessFlags dstAccessMask = 0;

            // Source access mask based on old layout
            switch (oldVkLayout) {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    srcAccessMask = 0;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                    break;
            }

            // Destination access mask based on new layout
            switch (newVkLayout) {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    dstAccessMask = 0; // Present doesn't need access mask
                    break;
                default:
                    dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                    break;
            }

            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;

            // Determine pipeline stages
            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

            // Source stage based on old layout
            switch (oldVkLayout) {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;
                default:
                    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    break;
            }

            // Destination stage based on new layout
            switch (newVkLayout) {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    break;
                default:
                    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    break;
            }

            vkCmdPipelineBarrier(
                m_VkCommandBuffer,
                sourceStage,
                destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            // Update the tracked layout after transition
            vkImage->SetCurrentLayout(newVkLayout);
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
            : m_Context(context), m_Image(image), m_CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED) {
        }

        VulkanImage::VulkanImage(DeviceContext* context, VkImage image, VkFormat format)
            : m_Context(context), m_Image(nullptr), m_VkImage(image), m_VkFormat(format), m_CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED) {
            // For swapchain images, we don't own the VkImage, just wrap it
            // Swapchain images start as UNDEFINED layout
        }

        VulkanImage::~VulkanImage() = default;

        uint32_t VulkanImage::GetWidth() const {
            if (m_Image) {
                return m_Image->GetWidth();
            }
            // For swapchain images, we don't have width/height stored
            // This is a limitation - swapchain images should be accessed via ISwapchain::GetExtent
            return 0;
        }

        uint32_t VulkanImage::GetHeight() const {
            if (m_Image) {
                return m_Image->GetHeight();
            }
            // For swapchain images, we don't have width/height stored
            return 0;
        }

        RHI::Format VulkanImage::GetFormat() const {
            VkFormat vkFormat;
            if (m_Image) {
                vkFormat = m_Image->GetFormat();
            } else {
                vkFormat = m_VkFormat; // For swapchain images
            }
            
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
            if (m_Image) {
                return m_Image->GetImage();
            }
            return m_VkImage; // For swapchain images
        }

        // VulkanSwapchain implementation
        VulkanSwapchain::VulkanSwapchain(DeviceContext* context, Core::Window* window, VkSurfaceKHR surface)
            : m_Context(context), m_Window(window), m_Surface(surface),
              m_Swapchain(VK_NULL_HANDLE), m_ImageFormat(VK_FORMAT_UNDEFINED) {
            m_Extent = {0, 0};
        }

        VulkanSwapchain::~VulkanSwapchain() {
            CleanupSwapchain();
        }

        bool VulkanSwapchain::Create() {
            CreateSwapchain();
            CreateImageViews();
            return m_Swapchain != VK_NULL_HANDLE;
        }

        bool VulkanSwapchain::Recreate() {
            // Save old swapchain before cleanup (needed for proper recreation)
            // CreateSwapchain() will handle destroying the old swapchain after creating the new one
            VkSwapchainKHR oldSwapchain = m_Swapchain;
            
            // Cleanup image views and wrappers, but keep old swapchain handle
            // The old swapchain will be passed to CreateSwapchain() and destroyed there
            m_ImageWrappers.clear();
            for (auto imageView : m_ImageViews) {
                vkDestroyImageView(m_Context->GetDevice(), imageView, nullptr);
            }
            m_ImageViews.clear();
            
            // Create new swapchain (it will use oldSwapchain internally and destroy it)
            // Note: We don't set m_Swapchain to VK_NULL_HANDLE because CreateSwapchain() needs it
            try {
                CreateSwapchain();
                CreateImageViews();
                return m_Swapchain != VK_NULL_HANDLE;
            } catch (const std::exception& e) {
                std::cerr << "Failed to recreate swapchain: " << e.what() << std::endl;
                // If recreation failed, try to restore old swapchain if it's still valid
                if (oldSwapchain != VK_NULL_HANDLE && m_Swapchain == VK_NULL_HANDLE) {
                    m_Swapchain = oldSwapchain;
                }
                return false;
            }
        }

        bool VulkanSwapchain::AcquireNextImage(RHI::SemaphoreHandle semaphore, RHI::FenceHandle fence, uint32_t& imageIndex) {
            VkResult result = vkAcquireNextImageKHR(m_Context->GetDevice(), m_Swapchain, UINT64_MAX,
                                                    static_cast<VkSemaphore>(semaphore),
                                                    static_cast<VkFence>(fence),
                                                    &imageIndex);
            // Return true for success or suboptimal (swapchain needs recreation but can still be used)
            return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
        }

        bool VulkanSwapchain::Present(uint32_t imageIndex, const std::vector<RHI::SemaphoreHandle>& waitSemaphores) {
            if (m_Swapchain == VK_NULL_HANDLE) {
                return false;
            }

            std::vector<VkSemaphore> vkWaitSemaphores;
            vkWaitSemaphores.reserve(waitSemaphores.size());
            for (auto semaphore : waitSemaphores) {
                vkWaitSemaphores.push_back(static_cast<VkSemaphore>(semaphore));
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size());
            presentInfo.pWaitSemaphores = vkWaitSemaphores.empty() ? nullptr : vkWaitSemaphores.data();
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &m_Swapchain;
            presentInfo.pImageIndices = &imageIndex;

            return vkQueuePresentKHR(m_Context->GetPresentQueue(), &presentInfo) == VK_SUCCESS;
        }

        uint32_t VulkanSwapchain::GetImageCount() const {
            return static_cast<uint32_t>(m_Images.size());
        }

        RHI::Format VulkanSwapchain::GetImageFormat() const {
            // Convert VkFormat to RHI::Format
            switch (m_ImageFormat) {
                case VK_FORMAT_B8G8R8A8_UNORM: return RHI::Format::B8G8R8A8_UNORM;
                case VK_FORMAT_B8G8R8A8_SRGB: return RHI::Format::B8G8R8A8_SRGB;
                case VK_FORMAT_R8G8B8A8_UNORM: return RHI::Format::R8G8B8A8_UNORM;
                case VK_FORMAT_R8G8B8A8_SRGB: return RHI::Format::R8G8B8A8_SRGB;
                default: return RHI::Format::Undefined;
            }
        }

        void VulkanSwapchain::GetExtent(uint32_t& width, uint32_t& height) const {
            width = m_Extent.width;
            height = m_Extent.height;
        }

        RHI::IImage* VulkanSwapchain::GetImage(uint32_t index) {
            if (index >= m_Images.size()) {
                return nullptr;
            }

            // Ensure enough image wrappers
            while (m_ImageWrappers.size() <= index) {
                m_ImageWrappers.push_back(nullptr);
            }

            // Create wrapper if not exists
            if (!m_ImageWrappers[index]) {
                // Create a minimal wrapper for swapchain image
                // Note: Swapchain images are owned by the swapchain, so we don't manage their lifetime
                // We just need to wrap the VkImage handle
                m_ImageWrappers[index] = std::make_unique<VulkanImage>(m_Context, m_Images[index], m_ImageFormat);
            }

            return m_ImageWrappers[index].get();
        }

        void VulkanSwapchain::CreateSwapchain() {
            VkSurfaceCapabilitiesKHR capabilities;
            VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context->GetPhysicalDevice(), m_Surface, &capabilities);
            if (result != VK_SUCCESS) {
                std::cerr << "Failed to get surface capabilities: " << result << std::endl;
                throw std::runtime_error("Failed to get surface capabilities!");
            }
            
            // Debug: Log surface capabilities
            std::cout << "Surface capabilities:" << std::endl;
            std::cout << "  minImageExtent: " << capabilities.minImageExtent.width << "x" << capabilities.minImageExtent.height << std::endl;
            std::cout << "  maxImageExtent: " << capabilities.maxImageExtent.width << "x" << capabilities.maxImageExtent.height << std::endl;
            std::cout << "  currentExtent: " << capabilities.currentExtent.width << "x" << capabilities.currentExtent.height << std::endl;

            // Check if surface is valid and not lost
            if (capabilities.maxImageExtent.width == 0 || capabilities.maxImageExtent.height == 0) {
                std::cerr << "Surface capabilities indicate invalid extent (window may be minimized)" << std::endl;
                throw std::runtime_error("Surface extent is invalid - window may be minimized!");
            }

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(), m_Surface, &formatCount, nullptr);
            if (formatCount == 0) {
                throw std::runtime_error("No surface formats available!");
            }
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context->GetPhysicalDevice(), m_Surface, &formatCount, formats.data());

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
            if (presentModeCount == 0) {
                throw std::runtime_error("No present modes available!");
            }
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context->GetPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

            VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(formats);
            VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
            VkExtent2D extent = ChooseSwapExtent(capabilities);

            // Validate extent
            if (extent.width == 0 || extent.height == 0) {
                std::cerr << "Invalid swapchain extent: " << extent.width << "x" << extent.height << std::endl;
                throw std::runtime_error("Invalid swapchain extent - window size may be invalid!");
            }

            uint32_t imageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }

            // Save old swapchain for proper recreation
            VkSwapchainKHR oldSwapchain = m_Swapchain;

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = m_Surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = {m_Context->GetGraphicsQueueFamily(), m_Context->GetPresentQueueFamily()};
            if (m_Context->GetGraphicsQueueFamily() != m_Context->GetPresentQueueFamily()) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0;
                createInfo.pQueueFamilyIndices = nullptr;
            }

            createInfo.preTransform = capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = oldSwapchain; // Pass old swapchain for proper recreation

            VkResult createResult = vkCreateSwapchainKHR(m_Context->GetDevice(), &createInfo, nullptr, &m_Swapchain);
            if (createResult != VK_SUCCESS) {
                std::string errorMsg = "Failed to create swap chain! Error code: ";
                errorMsg += std::to_string(createResult);
                
                // Add specific error descriptions
                switch (createResult) {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_HOST_MEMORY)";
                        break;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_DEVICE_MEMORY)";
                        break;
                    case VK_ERROR_DEVICE_LOST:
                        errorMsg += " (VK_ERROR_DEVICE_LOST)";
                        break;
                    case VK_ERROR_SURFACE_LOST_KHR:
                        errorMsg += " (VK_ERROR_SURFACE_LOST_KHR - Surface lost)";
                        break;
                    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                        errorMsg += " (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)";
                        break;
                    case VK_ERROR_INITIALIZATION_FAILED:
                        errorMsg += " (VK_ERROR_INITIALIZATION_FAILED)";
                        break;
                    default:
                        errorMsg += " (Unknown error)";
                        break;
                }
                
                std::cerr << errorMsg << std::endl;
                std::cerr << "Swapchain creation details:" << std::endl;
                std::cerr << "  Extent: " << extent.width << "x" << extent.height << std::endl;
                std::cerr << "  Format: " << surfaceFormat.format << std::endl;
                std::cerr << "  Image count: " << imageCount << std::endl;
                std::cerr << "  Old swapchain: " << oldSwapchain << std::endl;
                
                throw std::runtime_error(errorMsg);
            }

            // Destroy old swapchain after creating new one (Vulkan spec requirement)
            if (oldSwapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_Context->GetDevice(), oldSwapchain, nullptr);
            }

            vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &imageCount, nullptr);
            m_Images.resize(imageCount);
            vkGetSwapchainImagesKHR(m_Context->GetDevice(), m_Swapchain, &imageCount, m_Images.data());

            m_ImageFormat = surfaceFormat.format;
            m_Extent = extent;
        }

        void VulkanSwapchain::CreateImageViews() {
            m_ImageViews.resize(m_Images.size());

            for (size_t i = 0; i < m_Images.size(); i++) {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = m_Images[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = m_ImageFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(m_Context->GetDevice(), &createInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create image views!");
                }
            }
        }

        void VulkanSwapchain::CleanupSwapchain() {
            m_ImageWrappers.clear();

            for (auto imageView : m_ImageViews) {
                vkDestroyImageView(m_Context->GetDevice(), imageView, nullptr);
            }
            m_ImageViews.clear();

            if (m_Swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_Context->GetDevice(), m_Swapchain, nullptr);
                m_Swapchain = VK_NULL_HANDLE;
            }
        }

        VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

        VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != UINT32_MAX) {
                return capabilities.currentExtent;
            } else {
                int width, height;
                m_Window->GetFramebufferSize(&width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
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
