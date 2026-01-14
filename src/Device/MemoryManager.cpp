#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <stdexcept>
#include <cstring>

namespace FirstEngine {
    namespace Device {

        // Buffer 实现
        Buffer::Buffer(DeviceContext* context) 
            : m_Context(context), m_Buffer(VK_NULL_HANDLE), m_Size(0), m_MappedData(nullptr) {
            m_Allocation.memory = VK_NULL_HANDLE;
            m_Allocation.offset = 0;
            m_Allocation.size = 0;
            m_Allocation.memoryTypeIndex = 0;
        }

        Buffer::~Buffer() {
            Destroy();
        }

        bool Buffer::Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
            m_Size = size;

            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(m_Context->GetDevice(), &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS) {
                return false;
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_Context->GetDevice(), m_Buffer, &memRequirements);

            MemoryManager memoryManager(m_Context);
            if (!memoryManager.AllocateMemory(memRequirements, properties, m_Allocation)) {
                vkDestroyBuffer(m_Context->GetDevice(), m_Buffer, nullptr);
                m_Buffer = VK_NULL_HANDLE;
                return false;
            }

            if (vkBindBufferMemory(m_Context->GetDevice(), m_Buffer, m_Allocation.memory, m_Allocation.offset) != VK_SUCCESS) {
                memoryManager.FreeMemory(m_Allocation);
                vkDestroyBuffer(m_Context->GetDevice(), m_Buffer, nullptr);
                m_Buffer = VK_NULL_HANDLE;
                return false;
            }

            return true;
        }

        void Buffer::Destroy() {
            if (m_MappedData) {
                Unmap();
            }

            if (m_Buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_Context->GetDevice(), m_Buffer, nullptr);
                m_Buffer = VK_NULL_HANDLE;
            }

            if (m_Allocation.memory != VK_NULL_HANDLE) {
                MemoryManager memoryManager(m_Context);
                memoryManager.FreeMemory(m_Allocation);
                m_Allocation.memory = VK_NULL_HANDLE;
            }
        }

        void* Buffer::Map(VkDeviceSize offset, VkDeviceSize size) {
            if (m_MappedData) {
                return m_MappedData;
            }

            if (size == VK_WHOLE_SIZE) {
                size = m_Size - offset;
            }

            if (vkMapMemory(m_Context->GetDevice(), m_Allocation.memory, 
                           m_Allocation.offset + offset, size, 0, &m_MappedData) != VK_SUCCESS) {
                return nullptr;
            }

            return m_MappedData;
        }

        void Buffer::Unmap() {
            if (m_MappedData) {
                vkUnmapMemory(m_Context->GetDevice(), m_Allocation.memory);
                m_MappedData = nullptr;
            }
        }

        void Buffer::UpdateData(const void* data, VkDeviceSize size, VkDeviceSize offset) {
            void* mapped = Map(offset, size);
            if (mapped) {
                memcpy(mapped, data, size);
                Unmap();
            }
        }

        // Image 实现
        Image::Image(DeviceContext* context)
            : m_Context(context), m_Image(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE),
              m_Format(VK_FORMAT_UNDEFINED), m_Width(0), m_Height(0), m_MipLevels(0) {
            m_Allocation.memory = VK_NULL_HANDLE;
            m_Allocation.offset = 0;
            m_Allocation.size = 0;
            m_Allocation.memoryTypeIndex = 0;
        }

        Image::~Image() {
            Destroy();
        }

        bool Image::Create(uint32_t width, uint32_t height, uint32_t mipLevels,
                          VkSampleCountFlagBits numSamples, VkFormat format,
                          VkImageTiling tiling, VkImageUsageFlags usage,
                          VkMemoryPropertyFlags properties) {
            m_Width = width;
            m_Height = height;
            m_MipLevels = mipLevels;
            m_Format = format;

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = numSamples;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(m_Context->GetDevice(), &imageInfo, nullptr, &m_Image) != VK_SUCCESS) {
                return false;
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(m_Context->GetDevice(), m_Image, &memRequirements);

            MemoryManager memoryManager(m_Context);
            if (!memoryManager.AllocateMemory(memRequirements, properties, m_Allocation)) {
                vkDestroyImage(m_Context->GetDevice(), m_Image, nullptr);
                m_Image = VK_NULL_HANDLE;
                return false;
            }

            if (vkBindImageMemory(m_Context->GetDevice(), m_Image, m_Allocation.memory, m_Allocation.offset) != VK_SUCCESS) {
                memoryManager.FreeMemory(m_Allocation);
                vkDestroyImage(m_Context->GetDevice(), m_Image, nullptr);
                m_Image = VK_NULL_HANDLE;
                return false;
            }

            return true;
        }

        bool Image::CreateImageView(VkImageViewType viewType, VkFormat format,
                                   VkImageAspectFlags aspectFlags) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Image;
            viewInfo.viewType = viewType;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_MipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Context->GetDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        void Image::Destroy() {
            if (m_ImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Context->GetDevice(), m_ImageView, nullptr);
                m_ImageView = VK_NULL_HANDLE;
            }

            if (m_Image != VK_NULL_HANDLE) {
                vkDestroyImage(m_Context->GetDevice(), m_Image, nullptr);
                m_Image = VK_NULL_HANDLE;
            }

            if (m_Allocation.memory != VK_NULL_HANDLE) {
                MemoryManager memoryManager(m_Context);
                memoryManager.FreeMemory(m_Allocation);
                m_Allocation.memory = VK_NULL_HANDLE;
            }
        }

        // MemoryManager 实现
        MemoryManager::MemoryManager(DeviceContext* context) : m_Context(context) {
        }

        MemoryManager::~MemoryManager() {
        }

        uint32_t MemoryManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(m_Context->GetPhysicalDevice(), &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((typeFilter & (1 << i)) && 
                    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }

            throw std::runtime_error("Failed to find suitable memory type!");
        }

        bool MemoryManager::AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties,
                                          MemoryAllocation& allocation) {
            uint32_t memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = requirements.size;
            allocInfo.memoryTypeIndex = memoryTypeIndex;

            if (vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &allocation.memory) != VK_SUCCESS) {
                return false;
            }

            allocation.offset = 0;
            allocation.size = requirements.size;
            allocation.memoryTypeIndex = memoryTypeIndex;

            return true;
        }

        void MemoryManager::FreeMemory(const MemoryAllocation& allocation) {
            if (allocation.memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_Context->GetDevice(), allocation.memory, nullptr);
            }
        }

        std::unique_ptr<Buffer> MemoryManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                                            VkMemoryPropertyFlags properties) {
            auto buffer = std::make_unique<Buffer>(m_Context);
            if (!buffer->Create(size, usage, properties)) {
                return nullptr;
            }
            return buffer;
        }

        std::unique_ptr<Image> MemoryManager::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                                                          VkSampleCountFlagBits numSamples, VkFormat format,
                                                          VkImageTiling tiling, VkImageUsageFlags usage,
                                                          VkMemoryPropertyFlags properties) {
            auto image = std::make_unique<Image>(m_Context);
            if (!image->Create(width, height, mipLevels, numSamples, format, tiling, usage, properties)) {
                return nullptr;
            }
            return image;
        }

    } // namespace Device
} // namespace FirstEngine
