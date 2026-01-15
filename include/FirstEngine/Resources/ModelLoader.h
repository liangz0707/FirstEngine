#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {
        FE_RESOURCES_API struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;
        };

        FE_RESOURCES_API struct Mesh {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            std::string materialName;
        };

        FE_RESOURCES_API struct Bone {
            std::string name;
            glm::mat4 offsetMatrix;
            int parentIndex;
        };

        FE_RESOURCES_API struct Model {
            std::vector<Mesh> meshes;
            std::vector<Bone> bones;
            std::string name;
        };

        // Forward declarations
        class ResourceManager;

        class FE_RESOURCES_API ModelLoader {
        public:
            // Load result structure - contains only Metadata
            // Model is a logical collection of Mesh and Material resources (scene rendering unit)
            // Dependencies (Mesh, Material, Texture ResourceIDs) are stored in metadata.dependencies
            // Loader only loads current resource metadata, dependencies are handled by Resource::Load
            struct LoadResult {
                ResourceMetadata metadata;  // Metadata (name, ID, dependencies with ResourceIDs, etc.)
                bool success = false;
            };

            // Load model by ResourceID (logical model - only XML metadata)
            // ResourceManager is used internally for caching and dependency resolution
            static LoadResult Load(ResourceID id);

            // Save model to XML file
            static bool Save(const std::string& xmlFilePath,
                           const std::string& name,
                           ResourceID id,
                           const std::vector<std::pair<uint32_t, ResourceID>>& meshIndices,
                           const std::vector<std::pair<uint32_t, ResourceID>>& materialIndices,
                           const std::vector<std::pair<std::string, ResourceID>>& textureSlots,
                           const std::vector<ResourceDependency>& dependencies);

            // Utility function: Load model geometry from file (for tools/import, not used by Resource system)
            // Note: This is a legacy utility function. MeshLoader now handles mesh geometry loading.
            static Model LoadFromFile(const std::string& filepath);

            static bool IsFormatSupported(const std::string& filepath);

            static std::vector<std::string> GetSupportedFormats();
        };
    }
}
