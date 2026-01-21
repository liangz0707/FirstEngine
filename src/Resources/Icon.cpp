#include "FirstEngine/Resources/Icon.h"
#include "FirstEngine/Resources/ImageLoader.h"
#include "FirstEngine/Resources/ResourceTypeEnum.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        Icon::Icon() : m_Image(nullptr), m_ImageView(nullptr), m_Width(0), m_Height(0), m_OwnsImage(false) {
        }

        Icon::~Icon() {
            Clear();
        }

        bool Icon::LoadFromFile(const std::string& filePath) {
            Clear();

            // Load image data using ImageLoader
            ImageLoader::ImageData imageData = ImageLoader::LoadFromFile(filePath);
            if (imageData.data.empty()) {
                return false;
            }

            m_Width = imageData.width;
            m_Height = imageData.height;

            // Note: Creating RHI image requires a device
            // For now, we just store the image data
            // The actual RHI image creation should be done when a device is available
            // This will be handled by the caller or IconManager

            return true;
        }

        bool Icon::LoadFromMemory(const void* data, size_t size, const std::string& formatHint) {
            Clear();

            // Load image data from memory
            const uint8_t* dataPtr = static_cast<const uint8_t*>(data);
            ImageLoader::ImageData imageData = ImageLoader::LoadFromMemory(dataPtr, size);
            if (imageData.data.empty()) {
                return false;
            }

            m_Width = imageData.width;
            m_Height = imageData.height;

            return true;
        }

        bool Icon::CreateFromImage(RHI::IImage* image) {
            if (!image) {
                return false;
            }

            Clear();

            m_Image = image;
            m_Width = image->GetWidth();
            m_Height = image->GetHeight();
            m_OwnsImage = false; // Don't own the image, it's managed elsewhere

            return true;
        }

        bool Icon::GenerateThumbnail(uint32_t targetWidth, uint32_t targetHeight) {
            if (!m_Image || m_Width == 0 || m_Height == 0) {
                return false;
            }

            // Thumbnail generation would require image resizing
            // This is a placeholder - actual implementation would use image processing library
            // For now, just return success if dimensions are acceptable
            return true;
        }

        void Icon::Clear() {
            // Only destroy image if we own it
            if (m_OwnsImage && m_Image) {
                // Image destruction should be handled by RHI device
                // For now, just clear the pointer
                m_Image = nullptr;
            }

            if (m_ImageView) {
                // ImageView destruction should be handled by RHI device
                m_ImageView = nullptr;
            }

            m_Width = 0;
            m_Height = 0;
            m_OwnsImage = false;
        }

        // IconManager implementation
        IconManager& IconManager::GetInstance() {
            static IconManager instance;
            return instance;
        }

        Icon* IconManager::GetTypeIcon(ResourceType type) {
            auto it = m_TypeIcons.find(type);
            if (it != m_TypeIcons.end()) {
                return it->second.get();
            }

            // Create default icon for type (placeholder)
            // In a real implementation, this would load a default icon image
            auto icon = std::make_unique<Icon>();
            Icon* iconPtr = icon.get();
            m_TypeIcons[type] = std::move(icon);

            return iconPtr;
        }

        Icon* IconManager::GetResourceIcon(const std::string& resourcePath) {
            auto it = m_PathIcons.find(resourcePath);
            if (it != m_PathIcons.end()) {
                return it->second.get();
            }

            // Try to load icon from file
            // For textures, try to load the texture as icon
            // For other resources, use type icon
            auto icon = std::make_unique<Icon>();
            if (icon->LoadFromFile(resourcePath)) {
                Icon* iconPtr = icon.get();
                m_PathIcons[resourcePath] = std::move(icon);
                return iconPtr;
            }

            // Fallback to type icon
            // Determine type from path or use Unknown
            ResourceType type = ResourceType::Unknown;
            std::string ext = resourcePath.substr(resourcePath.find_last_of('.'));
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".dds" || ext == ".hdr") {
                type = ResourceType::Texture;
            } else if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".3ds") {
                type = ResourceType::Model;
            }

            return GetTypeIcon(type);
        }

        Icon* IconManager::GetResourceIcon(ResourceID resourceID) {
            auto it = m_IDIcons.find(resourceID);
            if (it != m_IDIcons.end()) {
                return it->second.get();
            }

            // For now, return nullptr - would need ResourceIDManager to resolve path
            return nullptr;
        }

        void IconManager::RegisterIcon(const std::string& resourcePath, std::unique_ptr<Icon> icon) {
            m_PathIcons[resourcePath] = std::move(icon);
        }

        void IconManager::RegisterIcon(ResourceID resourceID, std::unique_ptr<Icon> icon) {
            m_IDIcons[resourceID] = std::move(icon);
        }

        void IconManager::LoadDefaultIcons() {
            // Load default icons for each resource type
            // This would load icon images from a default icons directory
            // For now, just create placeholder icons
        }

        void IconManager::Clear() {
            m_TypeIcons.clear();
            m_PathIcons.clear();
            m_IDIcons.clear();
        }

    } // namespace Resources
} // namespace FirstEngine
