#pragma once

#include "FirstEngine/Resources/Export.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {
        struct FE_RESOURCES_API Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoord;
        };

        struct FE_RESOURCES_API Mesh {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            std::string materialName;
        };

        struct FE_RESOURCES_API Bone {
            std::string name;
            glm::mat4 offsetMatrix;
            int parentIndex;
        };

        struct FE_RESOURCES_API Model {
            std::vector<Mesh> meshes;
            std::vector<Bone> bones;
            std::string name;
        };

        class FE_RESOURCES_API ModelLoader {
        public:
            // 从文件加载模型
            static Model LoadFromFile(const std::string& filepath);

            // 支持的格式
            static bool IsFormatSupported(const std::string& filepath);

            // 获取支持的格式列表
            static std::vector<std::string> GetSupportedFormats();
        };
    }
}
