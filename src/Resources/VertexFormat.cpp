#include "FirstEngine/Resources/VertexFormat.h"
#include <algorithm>
#include <sstream>

namespace FirstEngine {
    namespace Resources {

        void VertexFormat::AddAttribute(VertexAttributeType type, uint32_t location) {
            // Check if attribute already exists
            for (const auto& attr : m_Attributes) {
                if (attr.type == type) {
                    return; // Already exists
                }
            }

            VertexAttribute attr;
            attr.type = type;
            attr.offset = m_Stride;
            attr.size = GetVertexAttributeSize(type);
            attr.location = (location != UINT32_MAX) ? location : static_cast<uint32_t>(m_Attributes.size());

            m_Attributes.push_back(attr);
            UpdateStride();
        }

        bool VertexFormat::HasAttribute(VertexAttributeType type) const {
            for (const auto& attr : m_Attributes) {
                if (attr.type == type) {
                    return true;
                }
            }
            return false;
        }

        const VertexAttribute* VertexFormat::GetAttribute(VertexAttributeType type) const {
            for (const auto& attr : m_Attributes) {
                if (attr.type == type) {
                    return &attr;
                }
            }
            return nullptr;
        }

        bool VertexFormat::MatchesShaderInputs(const std::vector<VertexAttributeType>& requiredAttributes) const {
            for (VertexAttributeType required : requiredAttributes) {
                if (!HasAttribute(required)) {
                    return false;
                }
            }
            return true;
        }

        void VertexFormat::UpdateStride() {
            m_Stride = 0;
            for (const auto& attr : m_Attributes) {
                m_Stride += attr.size;
            }
        }

        VertexFormat VertexFormat::CreatePositionOnly() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionNormal() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::Normal, 1);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionTexCoord() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::TexCoord0, 1);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionNormalTexCoord() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::Normal, 1);
            format.AddAttribute(VertexAttributeType::TexCoord0, 2);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionNormalTexCoordTangent() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::Normal, 1);
            format.AddAttribute(VertexAttributeType::TexCoord0, 2);
            format.AddAttribute(VertexAttributeType::Tangent, 3);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionNormalTexCoord0TexCoord1() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::Normal, 1);
            format.AddAttribute(VertexAttributeType::TexCoord0, 2);
            format.AddAttribute(VertexAttributeType::TexCoord1, 3);
            return format;
        }

        VertexFormat VertexFormat::CreatePositionNormalTexCoord0TexCoord1Tangent() {
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            format.AddAttribute(VertexAttributeType::Normal, 1);
            format.AddAttribute(VertexAttributeType::TexCoord0, 2);
            format.AddAttribute(VertexAttributeType::TexCoord1, 3);
            format.AddAttribute(VertexAttributeType::Tangent, 4);
            return format;
        }

        VertexFormat VertexFormat::CreateFromMeshData(
            bool hasNormals,
            bool hasTexCoords0,
            bool hasTexCoords1,
            bool hasTangents,
            bool hasColors0) {
            
            // Use predefined formats when possible for better code clarity and potential optimization
            // Check for common combinations first
            
            // Position only
            if (!hasNormals && !hasTexCoords0 && !hasTexCoords1 && !hasTangents && !hasColors0) {
                return CreatePositionOnly();
            }
            
            // Position + Normal
            if (hasNormals && !hasTexCoords0 && !hasTexCoords1 && !hasTangents && !hasColors0) {
                return CreatePositionNormal();
            }
            
            // Position + TexCoord0
            if (!hasNormals && hasTexCoords0 && !hasTexCoords1 && !hasTangents && !hasColors0) {
                return CreatePositionTexCoord();
            }
            
            // Position + Normal + TexCoord0
            if (hasNormals && hasTexCoords0 && !hasTexCoords1 && !hasTangents && !hasColors0) {
                return CreatePositionNormalTexCoord();
            }
            
            // Position + Normal + TexCoord0 + Tangent
            if (hasNormals && hasTexCoords0 && !hasTexCoords1 && hasTangents && !hasColors0) {
                return CreatePositionNormalTexCoordTangent();
            }
            
            // Position + Normal + TexCoord0 + TexCoord1
            if (hasNormals && hasTexCoords0 && hasTexCoords1 && !hasTangents && !hasColors0) {
                return CreatePositionNormalTexCoord0TexCoord1();
            }
            
            // Position + Normal + TexCoord0 + TexCoord1 + Tangent
            if (hasNormals && hasTexCoords0 && hasTexCoords1 && hasTangents && !hasColors0) {
                return CreatePositionNormalTexCoord0TexCoord1Tangent();
            }
            
            // For other combinations, build dynamically
            VertexFormat format;
            format.AddAttribute(VertexAttributeType::Position, 0);
            
            if (hasNormals) {
                format.AddAttribute(VertexAttributeType::Normal, format.GetAttributeCount());
            }
            
            if (hasTexCoords0) {
                format.AddAttribute(VertexAttributeType::TexCoord0, format.GetAttributeCount());
            }
            
            if (hasTexCoords1) {
                format.AddAttribute(VertexAttributeType::TexCoord1, format.GetAttributeCount());
            }
            
            if (hasTangents) {
                format.AddAttribute(VertexAttributeType::Tangent, format.GetAttributeCount());
            }
            
            if (hasColors0) {
                format.AddAttribute(VertexAttributeType::Color0, format.GetAttributeCount());
            }
            
            return format;
        }

        std::string VertexFormat::ToString() const {
            std::ostringstream oss;
            oss << "VertexFormat(stride=" << m_Stride << ", attributes=[";
            for (size_t i = 0; i < m_Attributes.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << GetVertexAttributeTypeName(m_Attributes[i].type);
            }
            oss << "])";
            return oss.str();
        }

        const char* GetVertexAttributeTypeName(VertexAttributeType type) {
            switch (type) {
                case VertexAttributeType::Position: return "Position";
                case VertexAttributeType::Normal: return "Normal";
                case VertexAttributeType::TexCoord0: return "TexCoord0";
                case VertexAttributeType::TexCoord1: return "TexCoord1";
                case VertexAttributeType::Tangent: return "Tangent";
                case VertexAttributeType::Color0: return "Color0";
                default: return "Unknown";
            }
        }

        uint32_t GetVertexAttributeSize(VertexAttributeType type) {
            switch (type) {
                case VertexAttributeType::Position: return sizeof(glm::vec3);  // 12 bytes
                case VertexAttributeType::Normal: return sizeof(glm::vec3);    // 12 bytes
                case VertexAttributeType::TexCoord0: return sizeof(glm::vec2);  // 8 bytes
                case VertexAttributeType::TexCoord1: return sizeof(glm::vec2); // 8 bytes
                case VertexAttributeType::Tangent: return sizeof(glm::vec4);   // 16 bytes
                case VertexAttributeType::Color0: return sizeof(glm::vec4);    // 16 bytes
                default: return 0;
            }
        }

    } // namespace Resources
} // namespace FirstEngine
