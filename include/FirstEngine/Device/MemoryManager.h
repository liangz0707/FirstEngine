#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Device {

        // Memory allocation information
        FE_DEVICE_API struct MemoryAllocation {
            VkDeviceMemory memory;
            VkDeviceSize offset;
            VkDeviceSize size;
            uint32_t memoryTypeIndex;
        };

        // Buffer wrapper
        class FE_DEVICE_API Buffer {
        public:
            Buffer(DeviceContext* context);
            ~Buffer();

            // Create buffer
            bool Create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
            
            // Destroy buffer
            void Destroy();

            // Map memory (for CPU access)
            void* Map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
            void Unmap();

            // Update data
            void UpdateData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

            VkBuffer GetBuffer() const { return m_Buffer; }
            VkDeviceSize GetSize() const { return m_Size; }
            const MemoryAllocation& GetAllocation() const { return m_Allocation; }

        private:
            DeviceContext* m_Context;
            VkBuffer m_Buffer;
            VkDeviceSize m_Size;
            MemoryAllocation m_Allocation;
            void* m_MappedData;
        };

        // Image wrapper
        class FE_DEVICE_API Image {
        public:
            Image(DeviceContext* context);
            ~Image();

            // Create image
            bool Create(uint32_t width, uint32_t height, uint32_t mipLevels,
                       VkSampleCountFlagBits numSamples, VkFormat format,
                       VkImageTiling tiling, VkImageUsageFlags usage,
                       VkMemoryPropertyFlags properties);

            // Create image view
            bool CreateImageView(VkImageViewType viewType, VkFormat format,
                                VkImageAspectFlags aspectFlags);

            // Destroy image
            void Destroy();

            VkImage GetImage() const { return m_Image; }
            VkImageView GetImageView() const { return m_ImageView; }
            VkFormat GetFormat() const { return m_Format; }
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            uint32_t GetMipLevels() const { return m_MipLevels; }
            const MemoryAllocation& GetAllocation() const { return m_Allocation; }

        private:
            DeviceContext* m_Context;
            VkImage m_Image;
            VkImageView m_ImageView;
            VkFormat m_Format;
            uint32_t m_Width;
            uint32_t m_Height;
            uint32_t m_MipLevels;
            MemoryAllocation m_Allocation;
        };

        // Memory manager
        class FE_DEVICE_API MemoryManager {
        public:
            MemoryManager(DeviceContext* context);
            ~MemoryManager();

            // Find suitable memory type
            uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

            // Allocate memory
            bool AllocateMemory(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties,
                               MemoryAllocation& allocation);

            // Free memory
            void FreeMemory(const MemoryAllocation& allocation);

            // Create buffer
            std::unique_ptr<Buffer> CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                                 VkMemoryPropertyFlags properties);

            // Create image
            std::unique_ptr<Image> CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                                              VkSampleCountFlagBits numSamples, VkFormat format,
                                              VkImageTiling tiling, VkImageUsageFlags usage,
                                              VkMemoryPropertyFlags properties);

        private:
            DeviceContext* m_Context;
        };

    } // namespace Device
} // namespace FirstEngine
