#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include <string>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        struct Vertex;
        struct Bone;
        class ResourceManager;
        struct ResourceMetadata;

        // Mesh loader - loads actual mesh geometry data (vertices, indices, bones) from XML and binary files
        // ResourceManager is used internally for caching, not exposed to Resource classes
        class FE_RESOURCES_API MeshLoader {
        public:
            // Load result structure - contains both Handle data and Metadata
            struct LoadResult {
                std::vector<Vertex> vertices;      // Handle data: actual vertex data
                std::vector<uint32_t> indices;     // Handle data: actual index data
                std::vector<Bone> bones;           // Handle data: bone/skeleton data
                uint32_t vertexStride = 0;         // Handle data: vertex stride
                std::string meshFile;              // Source mesh file path (for saving)
                ResourceMetadata metadata;         // Metadata (name, ID, dependencies, etc.)
                bool success = false;
            };

            // Load mesh by ResourceID
            // ResourceManager is used internally for caching and dependency resolution
            static LoadResult Load(ResourceID id);

            // Save mesh to XML file (similar to TextureLoader::Save)
            // XML contains metadata and source mesh file path (fbx, obj, etc.)
            static bool Save(const std::string& xmlFilePath,
                           const std::string& name,
                           ResourceID id,
                           const std::string& meshFile,
                           uint32_t vertexStride);

            // Check if format is supported
            static bool IsFormatSupported(const std::string& filepath);
            static std::vector<std::string> GetSupportedFormats();
        };

    } // namespace Resources
} // namespace FirstEngine
