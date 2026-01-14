#include "FirstEngine/Resources/ImageLoader.h"
#include <stdexcept>
#include <fstream>
#include <vector>
#include <cstring>

// Use stb_image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Use stb_image_write for image saving (optional)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace FirstEngine {
    namespace Resources {

        ImageData ImageLoader::LoadFromFile(const std::string& filepath) {
            ImageData result{};
            result.width = 0;
            result.height = 0;
            result.channels = 0;
            result.hasAlpha = false;

            int width, height, channels;
            unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

            if (!data) {
                throw std::runtime_error("Failed to load image: " + filepath + " - " + stbi_failure_reason());
            }

            result.width = static_cast<uint32_t>(width);
            result.height = static_cast<uint32_t>(height);
            result.channels = static_cast<uint32_t>(channels);
            result.hasAlpha = (channels == 4);

            // Copy data
            size_t dataSize = width * height * channels;
            result.data.resize(dataSize);
            std::memcpy(result.data.data(), data, dataSize);

            // Free memory allocated by stb_image
            stbi_image_free(data);

            return result;
        }

        ImageData ImageLoader::LoadFromMemory(const uint8_t* data, size_t size) {
            ImageData result{};
            result.width = 0;
            result.height = 0;
            result.channels = 0;
            result.hasAlpha = false;

            int width, height, channels;
            unsigned char* imageData = stbi_load_from_memory(
                data, static_cast<int>(size), &width, &height, &channels, 0);

            if (!imageData) {
                throw std::runtime_error("Failed to load image from memory: " + std::string(stbi_failure_reason()));
            }

            result.width = static_cast<uint32_t>(width);
            result.height = static_cast<uint32_t>(height);
            result.channels = static_cast<uint32_t>(channels);
            result.hasAlpha = (channels == 4);

            // Copy data
            size_t dataSize = width * height * channels;
            result.data.resize(dataSize);
            std::memcpy(result.data.data(), imageData, dataSize);

            stbi_image_free(imageData);

            return result;
        }

        ImageFormat ImageLoader::DetectFormat(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::binary);
            if (!file.is_open()) {
                return ImageFormat::Unknown;
            }

            uint8_t header[16];
            file.read(reinterpret_cast<char*>(header), 16);
            file.close();

            return DetectFormat(header, 16);
        }

        ImageFormat ImageLoader::DetectFormat(const uint8_t* data, size_t size) {
            if (size < 4) {
                return ImageFormat::Unknown;
            }

            // JPEG: FF D8 FF
            if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
                return ImageFormat::JPEG;
            }

            // PNG: 89 50 4E 47
            if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
                return ImageFormat::PNG;
            }

            // BMP: 42 4D
            if (data[0] == 0x42 && data[1] == 0x4D) {
                return ImageFormat::BMP;
            }

            // TGA: Check file extension or specific format
            // TGA format is complex, simplified handling here

            // DDS: 44 44 53 20 (DDS )
            if (size >= 4 && data[0] == 0x44 && data[1] == 0x44 && data[2] == 0x53 && data[3] == 0x20) {
                return ImageFormat::DDS;
            }

            // TIFF: 49 49 2A 00 (little-endian) or 4D 4D 00 2A (big-endian)
            if ((data[0] == 0x49 && data[1] == 0x49 && data[2] == 0x2A && data[3] == 0x00) ||
                (data[0] == 0x4D && data[1] == 0x4D && data[2] == 0x00 && data[3] == 0x2A)) {
                return ImageFormat::TIFF;
            }

            // HDR: #?RADIANCE or #?RGBE
            if (size >= 10 && data[0] == '#' && data[1] == '?') {
                return ImageFormat::HDR;
            }

            return ImageFormat::Unknown;
        }

        void ImageLoader::FreeImageData(ImageData& imageData) {
            imageData.data.clear();
            imageData.width = 0;
            imageData.height = 0;
            imageData.channels = 0;
            imageData.hasAlpha = false;
        }
    }
}
