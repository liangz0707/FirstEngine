#pragma once

#include "FirstEngine/Resources/Export.h"
#include <string>
#include <vector>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {
        struct FE_RESOURCES_API ImageData {
            std::vector<uint8_t> data;
            uint32_t width;
            uint32_t height;
            uint32_t channels;  // 1=grayscale, 3=RGB, 4=RGBA
            bool hasAlpha;
        };

        enum class FE_RESOURCES_API ImageFormat {
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
            // 从文件加载图像
            static ImageData LoadFromFile(const std::string& filepath);

            // 从内存加载图像
            static ImageData LoadFromMemory(const uint8_t* data, size_t size);

            // 检测图像格式
            static ImageFormat DetectFormat(const std::string& filepath);
            static ImageFormat DetectFormat(const uint8_t* data, size_t size);

            // 释放图像数据
            static void FreeImageData(ImageData& imageData);
        };
    }
}
