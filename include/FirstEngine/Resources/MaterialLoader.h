#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        class ResourceManager;
        struct ResourceMetadata;
        struct ResourceDependency;

        // Material loader - loads material metadata from XML
        // ResourceManager is used internally for caching, not exposed to Resource classes
        class FE_RESOURCES_API MaterialLoader {
        public:
            // Load result structure - contains both Handle data and Metadata
            // Loader only loads current resource data, dependencies are handled by Resource::Load
            struct LoadResult {
                std::string shaderName;                                    // Handle data
                std::unordered_map<std::string, MaterialParameterValue> parameters; // Handle data
                ResourceMetadata metadata;                                 // Metadata (name, ID, dependencies, etc.)
                bool success = false;
            };

            // Load material by ResourceID
            // ResourceManager is used internally for caching and dependency resolution
            static LoadResult Load(ResourceID id);

            // Save material to XML file
            static bool Save(const std::string& xmlFilePath,
                           const std::string& name,
                           ResourceID id,
                           const std::string& shaderName,
                           const std::unordered_map<std::string, MaterialParameterValue>& parameters,
                           const std::unordered_map<std::string, ResourceID>& textureSlots,
                           const std::vector<ResourceDependency>& dependencies);

            // Check if format is supported
            static bool IsFormatSupported(const std::string& filepath);
            static std::vector<std::string> GetSupportedFormats();
        };

    } // namespace Resources
} // namespace FirstEngine
