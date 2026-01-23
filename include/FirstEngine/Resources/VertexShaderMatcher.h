#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/VertexFormat.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <vector>
#include <string>

namespace FirstEngine {
    namespace Resources {

        // Vertex attribute to shader input matching strategy
        class FE_RESOURCES_API VertexShaderMatcher {
        public:
            // Match result
            struct MatchResult {
                bool isCompatible = false;
                std::vector<uint32_t> locationMapping;  // Maps vertex format attribute index to shader location
                std::vector<std::string> missingAttributes;  // Shader requires but vertex format doesn't have
                std::vector<std::string> unusedAttributes;   // Vertex format has but shader doesn't use
                std::string errorMessage;
            };

            // Match vertex format to shader stage inputs
            // Strategy: Check if vertex format has all required attributes for the shader
            static MatchResult Match(const VertexFormat& vertexFormat, 
                                    const Shader::ShaderReflection& shaderReflection);

            // Extract required vertex attributes from shader reflection
            static std::vector<VertexAttributeType> ExtractRequiredAttributes(
                const Shader::ShaderReflection& shaderReflection);

            // Map shader input name to vertex attribute type
            static VertexAttributeType MapShaderInputToAttributeType(const std::string& inputName);

            // Get attribute type from shader resource (based on name and location)
            static VertexAttributeType InferAttributeTypeFromShaderResource(
                const Shader::ShaderResource& resource);

        private:
            // Helper: Check if shader input name matches attribute type
            static bool NameMatchesAttributeType(const std::string& name, VertexAttributeType type);
        };

    } // namespace Resources
} // namespace FirstEngine
