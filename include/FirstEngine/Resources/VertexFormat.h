#pragma once

#include "FirstEngine/Resources/Export.h"
#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        // Vertex attribute types
        enum class VertexAttributeType : uint32_t {
            Position = 0,      // float3 (required)
            Normal = 1,        // float3 (optional)
            TexCoord0 = 2,    // float2 (optional)
            TexCoord1 = 3,    // float2 (optional)
            Tangent = 4,      // float4 (xyz=tangent, w=handedness) (optional)
            Color0 = 5,       // float4 (optional)
            // Add more as needed
        };

        // Vertex attribute description
        struct VertexAttribute {
            VertexAttributeType type;
            uint32_t offset;      // Offset in bytes from vertex start
            uint32_t size;        // Size in bytes
            uint32_t location;    // Shader location (for matching)
        };

        // Vertex format descriptor - describes the layout of a vertex
        class FE_RESOURCES_API VertexFormat {
        public:
            VertexFormat() = default;
            ~VertexFormat() = default;

            // Add an attribute to the format
            void AddAttribute(VertexAttributeType type, uint32_t location = UINT32_MAX);

            // Check if format has a specific attribute
            bool HasAttribute(VertexAttributeType type) const;

            // Get attribute by type
            const VertexAttribute* GetAttribute(VertexAttributeType type) const;

            // Get all attributes
            const std::vector<VertexAttribute>& GetAttributes() const { return m_Attributes; }

            // Calculate total vertex stride
            uint32_t GetStride() const { return m_Stride; }

            // Get attribute count
            size_t GetAttributeCount() const { return m_Attributes.size(); }

            // Check if this format matches shader requirements
            // Returns true if format has all required attributes for the shader
            bool MatchesShaderInputs(const std::vector<VertexAttributeType>& requiredAttributes) const;

            // Create common vertex formats
            static VertexFormat CreatePositionOnly();
            static VertexFormat CreatePositionNormal();
            static VertexFormat CreatePositionTexCoord();
            static VertexFormat CreatePositionNormalTexCoord();
            static VertexFormat CreatePositionNormalTexCoordTangent();
            static VertexFormat CreatePositionNormalTexCoord0TexCoord1();
            static VertexFormat CreatePositionNormalTexCoord0TexCoord1Tangent();

            // Create format from mesh data (detect available attributes)
            static VertexFormat CreateFromMeshData(
                bool hasNormals,
                bool hasTexCoords0,
                bool hasTexCoords1,
                bool hasTangents,
                bool hasColors0 = false
            );

            // Convert to string for debugging
            std::string ToString() const;

        private:
            std::vector<VertexAttribute> m_Attributes;
            uint32_t m_Stride = 0;

            void UpdateStride();
        };

        // Helper: Get attribute type name
        FE_RESOURCES_API const char* GetVertexAttributeTypeName(VertexAttributeType type);

        // Helper: Get attribute size in bytes
        FE_RESOURCES_API uint32_t GetVertexAttributeSize(VertexAttributeType type);

    } // namespace Resources
} // namespace FirstEngine
