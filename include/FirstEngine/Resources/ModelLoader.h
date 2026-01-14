#pragma once

#include "FirstEngine/Resources/Export.h"
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

        class FE_RESOURCES_API ModelLoader {
        public:
            static Model LoadFromFile(const std::string& filepath);

            static bool IsFormatSupported(const std::string& filepath);

            static std::vector<std::string> GetSupportedFormats();
        };
    }
}
