#pragma once

#include "FirstEngine/Resources/Export.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        // Material parameter value type - supports various data types
        struct FE_RESOURCES_API MaterialParameterValue {
            // Supported types
            enum class Type : uint32_t {
                Float = 0,
                Vec2 = 1,
                Vec3 = 2,
                Vec4 = 3,
                Int = 4,
                Int2 = 5,
                Int3 = 6,
                Int4 = 7,
                Bool = 8,
                Mat3 = 9,
                Mat4 = 10
            };

            Type type;
            std::vector<uint8_t> data; // Raw data storage

            MaterialParameterValue() : type(Type::Float), data(sizeof(float)) {}
            
            // Constructors for different types
            MaterialParameterValue(float value);
            MaterialParameterValue(const glm::vec2& value);
            MaterialParameterValue(const glm::vec3& value);
            MaterialParameterValue(const glm::vec4& value);
            MaterialParameterValue(int32_t value);
            MaterialParameterValue(const glm::ivec2& value);
            MaterialParameterValue(const glm::ivec3& value);
            MaterialParameterValue(const glm::ivec4& value);
            MaterialParameterValue(bool value);
            MaterialParameterValue(const glm::mat3& value);
            MaterialParameterValue(const glm::mat4& value);

            // Copy constructor
            MaterialParameterValue(const MaterialParameterValue& other) = default;
            MaterialParameterValue& operator=(const MaterialParameterValue& other) = default;

            // Getters
            float GetFloat() const;
            glm::vec2 GetVec2() const;
            glm::vec3 GetVec3() const;
            glm::vec4 GetVec4() const;
            int32_t GetInt() const;
            glm::ivec2 GetInt2() const;
            glm::ivec3 GetInt3() const;
            glm::ivec4 GetInt4() const;
            bool GetBool() const;
            glm::mat3 GetMat3() const;
            glm::mat4 GetMat4() const;

            // Get raw data pointer and size
            const void* GetData() const { return data.data(); }
            uint32_t GetDataSize() const { return static_cast<uint32_t>(data.size()); }
        };

        // Material parameters - key-value pairs
        using MaterialParameters = std::unordered_map<std::string, MaterialParameterValue>;

    } // namespace Resources
} // namespace FirstEngine
