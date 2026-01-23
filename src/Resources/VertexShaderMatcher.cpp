#include "FirstEngine/Resources/VertexShaderMatcher.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace FirstEngine {
    namespace Resources {

        VertexShaderMatcher::MatchResult VertexShaderMatcher::Match(
            const VertexFormat& vertexFormat,
            const Shader::ShaderReflection& shaderReflection) {
            
            MatchResult result;
            result.isCompatible = true;

            // Extract required attributes from shader
            std::vector<VertexAttributeType> requiredAttributes = ExtractRequiredAttributes(shaderReflection);

            // Check if vertex format has all required attributes
            for (VertexAttributeType required : requiredAttributes) {
                if (!vertexFormat.HasAttribute(required)) {
                    result.isCompatible = false;
                    result.missingAttributes.push_back(GetVertexAttributeTypeName(required));
                }
            }

            // Build location mapping (vertex format attribute -> shader location)
            // This assumes shader locations match attribute order, but we can refine this
            const auto& formatAttributes = vertexFormat.GetAttributes();
            for (size_t i = 0; i < formatAttributes.size(); ++i) {
                // Try to find matching shader input by location or name
                uint32_t shaderLocation = formatAttributes[i].location;
                
                // Find shader input with matching location
                bool found = false;
                for (const auto& input : shaderReflection.stage_inputs) {
                    if (input.location == shaderLocation) {
                        result.locationMapping.push_back(shaderLocation);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    // No matching shader input - this attribute is unused
                    result.unusedAttributes.push_back(GetVertexAttributeTypeName(formatAttributes[i].type));
                }
            }

            // Generate error message if incompatible
            if (!result.isCompatible) {
                std::ostringstream oss;
                oss << "Vertex format incompatible with shader. Missing attributes: ";
                for (size_t i = 0; i < result.missingAttributes.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << result.missingAttributes[i];
                }
                result.errorMessage = oss.str();
            }

            return result;
        }

        std::vector<VertexAttributeType> VertexShaderMatcher::ExtractRequiredAttributes(
            const Shader::ShaderReflection& shaderReflection) {
            
            std::vector<VertexAttributeType> required;

            for (const auto& input : shaderReflection.stage_inputs) {
                VertexAttributeType attrType = InferAttributeTypeFromShaderResource(input);
                if (attrType != VertexAttributeType::Position || 
                    std::find(required.begin(), required.end(), attrType) == required.end()) {
                    // Position is always required, others are optional but should be included if present
                    if (std::find(required.begin(), required.end(), attrType) == required.end()) {
                        required.push_back(attrType);
                    }
                }
            }

            // Ensure Position is always first
            auto posIt = std::find(required.begin(), required.end(), VertexAttributeType::Position);
            if (posIt == required.end()) {
                required.insert(required.begin(), VertexAttributeType::Position);
            } else if (posIt != required.begin()) {
                required.erase(posIt);
                required.insert(required.begin(), VertexAttributeType::Position);
            }

            return required;
        }

        VertexAttributeType VertexShaderMatcher::MapShaderInputToAttributeType(const std::string& inputName) {
            std::string lowerName = inputName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            // Common naming patterns
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::Position)) {
                return VertexAttributeType::Position;
            }
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::Normal)) {
                return VertexAttributeType::Normal;
            }
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::TexCoord0) ||
                lowerName.find("texcoord0") != std::string::npos ||
                lowerName.find("uv0") != std::string::npos ||
                lowerName.find("texcoord") != std::string::npos) {
                return VertexAttributeType::TexCoord0;
            }
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::TexCoord1) ||
                lowerName.find("texcoord1") != std::string::npos ||
                lowerName.find("uv1") != std::string::npos) {
                return VertexAttributeType::TexCoord1;
            }
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::Tangent)) {
                return VertexAttributeType::Tangent;
            }
            if (NameMatchesAttributeType(lowerName, VertexAttributeType::Color0) ||
                lowerName.find("color0") != std::string::npos ||
                lowerName.find("color") != std::string::npos) {
                return VertexAttributeType::Color0;
            }

            return VertexAttributeType::Position; // Default fallback
        }

        VertexAttributeType VertexShaderMatcher::InferAttributeTypeFromShaderResource(
            const Shader::ShaderResource& resource) {
            
            // First try to infer from name
            VertexAttributeType type = MapShaderInputToAttributeType(resource.name);
            
            // If name doesn't give clear indication, use location as hint
            // Common convention: location 0 = Position, 1 = Normal, 2 = TexCoord, etc.
            if (type == VertexAttributeType::Position && resource.location != 0) {
                switch (resource.location) {
                    case 0: return VertexAttributeType::Position;
                    case 1: return VertexAttributeType::Normal;
                    case 2: return VertexAttributeType::TexCoord0;
                    case 3: return VertexAttributeType::TexCoord1;
                    case 4: return VertexAttributeType::Tangent;
                    case 5: return VertexAttributeType::Color0;
                    default: return VertexAttributeType::Position;
                }
            }

            return type;
        }

        bool VertexShaderMatcher::NameMatchesAttributeType(const std::string& name, VertexAttributeType type) {
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            switch (type) {
                case VertexAttributeType::Position:
                    return lowerName.find("position") != std::string::npos ||
                           lowerName.find("pos") != std::string::npos;
                case VertexAttributeType::Normal:
                    return lowerName.find("normal") != std::string::npos ||
                           lowerName.find("norm") != std::string::npos;
                case VertexAttributeType::TexCoord0:
                    return lowerName.find("texcoord0") != std::string::npos ||
                           lowerName.find("uv0") != std::string::npos ||
                           (lowerName.find("texcoord") != std::string::npos && 
                            lowerName.find("texcoord1") == std::string::npos);
                case VertexAttributeType::TexCoord1:
                    return lowerName.find("texcoord1") != std::string::npos ||
                           lowerName.find("uv1") != std::string::npos;
                case VertexAttributeType::Tangent:
                    return lowerName.find("tangent") != std::string::npos;
                case VertexAttributeType::Color0:
                    return lowerName.find("color0") != std::string::npos ||
                           (lowerName.find("color") != std::string::npos && 
                            lowerName.find("color1") == std::string::npos);
                default:
                    return false;
            }
        }

    } // namespace Resources
} // namespace FirstEngine
