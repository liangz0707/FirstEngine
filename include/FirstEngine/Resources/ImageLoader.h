#pragma once

#include "FirstEngine/Resources/Export.h"
#include <string>
#include <vector>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {
        FE_RESOURCES_API struct ImageData {
            std::vector<uint8_t> data;
            uint32_t width;
            uint32_t height;
            uint32_t channels;  // 1=grayscale, 3=RGB, 4=RGBA
            bool hasAlpha;
        };

        FE_RESOURCES_API enum class ImageFormat {
            JPEG,
            PNG,
            BMP,
            TGA,
            DDS,
            TIFF,
            HDR,
            Unknown
        };

        class FE_RESOURCES_API ImageLoader {
        public:
            static ImageData LoadFromFile(const std::string& filepath);

            static ImageData LoadFromMemory(const uint8_t* data, size_t size);

            static ImageFormat DetectFormat(const std::string& filepath);
            static ImageFormat DetectFormat(const uint8_t* data, size_t size);

            static void FreeImageData(ImageData& imageData);
        };
    }
}
