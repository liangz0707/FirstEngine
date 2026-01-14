#pragma once

#include "FirstEngine/Resources/Export.h"
#include <vulkan/vulkan.h>
#include <string>

namespace FirstEngine {
    namespace Resources {
        FE_RESOURCES_API struct Texture {
            VkImage image;
            VkImageView imageView;
            VkDeviceMemory imageMemory;
            uint32_t width;
            uint32_t height;
        };

        class FE_RESOURCES_API TextureLoader {
        public:
            static Texture LoadTexture(VkDevice device, VkPhysicalDevice physicalDevice, 
                                      VkCommandPool commandPool, VkQueue graphicsQueue,
                                      const std::string& filepath);
            static void DestroyTexture(VkDevice device, const Texture& texture);
        };
    }
}
