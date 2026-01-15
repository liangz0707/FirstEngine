#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ImageLoader.h"
#include <string>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        class ResourceManager;
        struct ResourceMetadata;

        // Texture loader - loads texture metadata from XML and image data from image files
        // ResourceManager is used internally for caching, not exposed to Resource classes
        class FE_RESOURCES_API TextureLoader {
        public:
            // Load result structure - contains both Handle data and Metadata
            struct LoadResult {
                ImageData imageData;           // Handle data (actual image data)
                ResourceMetadata metadata;     // Metadata (name, ID, dependencies, etc.)
                bool success = false;
            };

            // Load texture by ResourceID
            // ResourceManager is used internally for caching and dependency resolution
            static LoadResult Load(ResourceID id);

            // Save texture to XML file
            static bool Save(const std::string& xmlFilePath,
                            const std::string& name,
                            ResourceID id,
                            const std::string& imageFilePath,
                            uint32_t width,
                            uint32_t height,
                            uint32_t channels,
                            bool hasAlpha);

            // Check if format is supported
            static bool IsFormatSupported(const std::string& filepath);
            static std::vector<std::string> GetSupportedFormats();
        };

    } // namespace Resources
} // namespace FirstEngine
