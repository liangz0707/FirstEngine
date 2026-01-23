#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypeEnum.h"
#include "FirstEngine/RHI/IImage.h"  // IImageView is defined in IImage.h
#include <string>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        // Icon class for handling icons in scene and resource browser
        // Supports loading icons from various image formats
        class FE_RESOURCES_API Icon {
        public:
            Icon();
            ~Icon();

            // Load icon from file (supports PNG, JPG, BMP, TGA, DDS, etc.)
            bool LoadFromFile(const std::string& filePath);

            // Create icon from RHI image (for rendering)
            bool CreateFromImage(RHI::IImage* image);

            // Get icon as RHI image (for rendering)
            RHI::IImage* GetImage() const { return m_Image; }
            RHI::IImageView* GetImageView() const { return m_ImageView; }

            // Get icon dimensions
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }

            // Check if icon is valid
            bool IsValid() const { return m_Image != nullptr; }

            // Generate thumbnail from image (resize to specified dimensions)
            bool GenerateThumbnail(uint32_t targetWidth, uint32_t targetHeight);

            // Clear icon data
            void Clear();

        private:
            RHI::IImage* m_Image = nullptr;
            RHI::IImageView* m_ImageView = nullptr;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            bool m_OwnsImage = false; // Whether we own the image (should destroy it)
        };

        // Icon manager for managing icon resources
        class FE_RESOURCES_API IconManager {
        public:
            static IconManager& GetInstance();

            // Get or create icon for a resource type
            Icon* GetTypeIcon(ResourceType type);

            // Get or create icon for a resource (by path or ID)
            Icon* GetResourceIcon(const std::string& resourcePath);
            Icon* GetResourceIcon(ResourceID resourceID);

            // Register custom icon for a resource
            void RegisterIcon(const std::string& resourcePath, std::unique_ptr<Icon> icon);
            void RegisterIcon(ResourceID resourceID, std::unique_ptr<Icon> icon);

            // Load default icons
            void LoadDefaultIcons();

            // Clear all icons
            void Clear();

        private:
            IconManager() = default;
            ~IconManager() = default;
            IconManager(const IconManager&) = delete;
            IconManager& operator=(const IconManager&) = delete;

            std::unordered_map<ResourceType, std::unique_ptr<Icon>> m_TypeIcons;
            std::unordered_map<std::string, std::unique_ptr<Icon>> m_PathIcons;
            std::unordered_map<ResourceID, std::unique_ptr<Icon>> m_IDIcons;
        };

    } // namespace Resources
} // namespace FirstEngine
