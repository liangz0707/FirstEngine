#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/VertexFormat.h"
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        // Legacy Vertex structure - kept for backward compatibility
        // NOTE: This structure is kept for backward compatibility. New code should use VertexFormat and flexible vertex data.
        // TODO: Consider deprecating this in a future version when all code migrates to VertexFormat
        FE_RESOURCES_API struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;
            glm::vec4 tangent;  // xyz = tangent, w = handedness (1.0 or -1.0)
        };

        // Forward declarations
        struct Bone;
        class ResourceManager;
        struct ResourceMetadata;

        // Mesh loader - loads actual mesh geometry data (vertices, indices, bones) from XML and binary files
        // ResourceManager is used internally for caching, not exposed to Resource classes
        class FE_RESOURCES_API MeshLoader {
        public:
            // Load result structure - contains both Handle data and Metadata
            struct LoadResult {
                // Flexible vertex data (raw bytes, format describes layout)
                std::vector<uint8_t> vertexData;   // Handle data: raw vertex data (flexible format)
                std::vector<uint32_t> indices;     // Handle data: actual index data
                std::vector<Bone> bones;           // Handle data: bone/skeleton data
                VertexFormat vertexFormat;         // Handle data: vertex format descriptor
                uint32_t vertexCount = 0;          // Handle data: number of vertices
                std::string meshFile;              // Source mesh file path (for saving)
                ResourceMetadata metadata;         // Metadata (name, ID, dependencies, etc.)
                bool success = false;

                // Legacy support: Convert to old Vertex format (if format matches)
                // NOTE: This method is kept for backward compatibility. New code should use vertexData and vertexFormat directly.
                // TODO: Consider deprecating this in a future version
                std::vector<Vertex> GetLegacyVertices() const;
            };

            // Load mesh by ResourceID
            // ResourceManager is used internally for caching and dependency resolution
            static LoadResult Load(ResourceID id);

            // Save mesh to XML file (similar to TextureLoader::Save)
            // XML contains metadata and source mesh file path (fbx, obj, etc.)
            // Note: vertexStride is calculated from mesh file, not stored in XML
            static bool Save(const std::string& xmlFilePath,
                           const std::string& name,
                           ResourceID id,
                           const std::string& meshFile);

            // Check if format is supported
            static bool IsFormatSupported(const std::string& filepath);
            static std::vector<std::string> GetSupportedFormats();
        };

    } // namespace Resources
} // namespace FirstEngine
