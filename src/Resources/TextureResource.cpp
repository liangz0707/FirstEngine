#include "FirstEngine/Resources/TextureResource.h"
#include "FirstEngine/Resources/TextureLoader.h"
#include "FirstEngine/Resources/ImageLoader.h"
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        TextureResource::TextureResource() {
            m_Metadata.refCount = 0;
            m_Metadata.isLoaded = false;
        }

        TextureResource::~TextureResource() = default;

        bool TextureResource::Initialize(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t channels, bool hasAlpha) {
            if (data.empty() || width == 0 || height == 0 || channels == 0) {
                return false;
            }

            m_Data = data;
            m_Width = width;
            m_Height = height;
            m_Channels = channels;
            m_HasAlpha = hasAlpha;

            m_Metadata.isLoaded = true;
            m_Metadata.fileSize = data.size();

            return true;
        }

        // IResourceProvider interface implementation
        bool TextureResource::IsFormatSupported(const std::string& filepath) const {
            ImageFormat format = ImageLoader::DetectFormat(filepath);
            return format != ImageFormat::Unknown;
        }

        std::vector<std::string> TextureResource::GetSupportedFormats() const {
            return { ".jpg", ".jpeg", ".png", ".bmp", ".tga", ".dds", ".tiff", ".hdr" };
        }

        ResourceLoadResult TextureResource::Load(ResourceID id) {
            if (id == InvalidResourceID) {
                return ResourceLoadResult::UnknownError;
            }

            // Set metadata ID
            m_Metadata.resourceID = id;

            // Call Loader::Load - ResourceManager is used internally by Loader for caching
            // Loader returns both Handle data (imageData) and Metadata
            TextureLoader::LoadResult loadResult = TextureLoader::Load(id);
            
            if (!loadResult.success) {
                return ResourceLoadResult::FileNotFound;
            }

            // Use returned Metadata
            m_Metadata = loadResult.metadata;
            m_Metadata.resourceID = id; // Ensure ID matches

            // Use returned Handle data (imageData) to initialize resource
            if (!Initialize(loadResult.imageData.data, 
                          loadResult.imageData.width, 
                          loadResult.imageData.height, 
                          loadResult.imageData.channels, 
                          loadResult.imageData.hasAlpha)) {
                ImageLoader::FreeImageData(loadResult.imageData);
                return ResourceLoadResult::InvalidFormat;
            }

            ImageLoader::FreeImageData(loadResult.imageData);
            m_Metadata.isLoaded = true;

            return ResourceLoadResult::Success;
        }

        ResourceLoadResult TextureResource::LoadFromMemory(const void* data, size_t size) {
            if (!data || size == 0) {
                return ResourceLoadResult::UnknownError;
            }

            // Load image data from memory
            ImageData imageData = ImageLoader::LoadFromMemory(static_cast<const uint8_t*>(data), size);
            if (imageData.data.empty()) {
                return ResourceLoadResult::InvalidFormat;
            }

            // Initialize this resource object
            if (!Initialize(imageData.data, imageData.width, imageData.height, imageData.channels, imageData.hasAlpha)) {
                ImageLoader::FreeImageData(imageData);
                return ResourceLoadResult::InvalidFormat;
            }

            m_Metadata.fileSize = imageData.data.size();
            m_Metadata.isLoaded = true;

            ImageLoader::FreeImageData(imageData);

            // Textures typically don't have dependencies
            LoadDependencies();

            return ResourceLoadResult::Success;
        }

        void TextureResource::LoadDependencies() {
            // Textures typically don't have dependencies
            // This method is here for interface compliance
        }

        bool TextureResource::Save(const std::string& xmlFilePath) const {
            // Determine image file path (relative to XML file)
            std::string imageFilePath = xmlFilePath;
            imageFilePath = fs::path(imageFilePath).replace_extension(".png").string(); // Default to PNG

            return TextureLoader::Save(xmlFilePath, m_Metadata.name, m_Metadata.resourceID,
                                      imageFilePath, m_Width, m_Height, m_Channels, m_HasAlpha);
        }

    } // namespace Resources
} // namespace FirstEngine
