#include "FirstEngine/Resources/MaterialParameter.h"
#include <cstring>
#include <cassert>

namespace FirstEngine {
    namespace Resources {

        MaterialParameterValue::MaterialParameterValue(float value) : type(Type::Float) {
            data.resize(sizeof(float));
            std::memcpy(data.data(), &value, sizeof(float));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::vec2& value) : type(Type::Vec2) {
            data.resize(sizeof(glm::vec2));
            std::memcpy(data.data(), &value, sizeof(glm::vec2));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::vec3& value) : type(Type::Vec3) {
            data.resize(sizeof(glm::vec3));
            std::memcpy(data.data(), &value, sizeof(glm::vec3));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::vec4& value) : type(Type::Vec4) {
            data.resize(sizeof(glm::vec4));
            std::memcpy(data.data(), &value, sizeof(glm::vec4));
        }

        MaterialParameterValue::MaterialParameterValue(int32_t value) : type(Type::Int) {
            data.resize(sizeof(int32_t));
            std::memcpy(data.data(), &value, sizeof(int32_t));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::ivec2& value) : type(Type::Int2) {
            data.resize(sizeof(glm::ivec2));
            std::memcpy(data.data(), &value, sizeof(glm::ivec2));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::ivec3& value) : type(Type::Int3) {
            data.resize(sizeof(glm::ivec3));
            std::memcpy(data.data(), &value, sizeof(glm::ivec3));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::ivec4& value) : type(Type::Int4) {
            data.resize(sizeof(glm::ivec4));
            std::memcpy(data.data(), &value, sizeof(glm::ivec4));
        }

        MaterialParameterValue::MaterialParameterValue(bool value) : type(Type::Bool) {
            data.resize(sizeof(bool));
            std::memcpy(data.data(), &value, sizeof(bool));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::mat3& value) : type(Type::Mat3) {
            data.resize(sizeof(glm::mat3));
            std::memcpy(data.data(), &value, sizeof(glm::mat3));
        }

        MaterialParameterValue::MaterialParameterValue(const glm::mat4& value) : type(Type::Mat4) {
            data.resize(sizeof(glm::mat4));
            std::memcpy(data.data(), &value, sizeof(glm::mat4));
        }

        float MaterialParameterValue::GetFloat() const {
            assert(type == Type::Float && data.size() >= sizeof(float));
            float value;
            std::memcpy(&value, data.data(), sizeof(float));
            return value;
        }

        glm::vec2 MaterialParameterValue::GetVec2() const {
            assert(type == Type::Vec2 && data.size() >= sizeof(glm::vec2));
            glm::vec2 value;
            std::memcpy(&value, data.data(), sizeof(glm::vec2));
            return value;
        }

        glm::vec3 MaterialParameterValue::GetVec3() const {
            assert(type == Type::Vec3 && data.size() >= sizeof(glm::vec3));
            glm::vec3 value;
            std::memcpy(&value, data.data(), sizeof(glm::vec3));
            return value;
        }

        glm::vec4 MaterialParameterValue::GetVec4() const {
            assert(type == Type::Vec4 && data.size() >= sizeof(glm::vec4));
            glm::vec4 value;
            std::memcpy(&value, data.data(), sizeof(glm::vec4));
            return value;
        }

        int32_t MaterialParameterValue::GetInt() const {
            assert(type == Type::Int && data.size() >= sizeof(int32_t));
            int32_t value;
            std::memcpy(&value, data.data(), sizeof(int32_t));
            return value;
        }

        glm::ivec2 MaterialParameterValue::GetInt2() const {
            assert(type == Type::Int2 && data.size() >= sizeof(glm::ivec2));
            glm::ivec2 value;
            std::memcpy(&value, data.data(), sizeof(glm::ivec2));
            return value;
        }

        glm::ivec3 MaterialParameterValue::GetInt3() const {
            assert(type == Type::Int3 && data.size() >= sizeof(glm::ivec3));
            glm::ivec3 value;
            std::memcpy(&value, data.data(), sizeof(glm::ivec3));
            return value;
        }

        glm::ivec4 MaterialParameterValue::GetInt4() const {
            assert(type == Type::Int4 && data.size() >= sizeof(glm::ivec4));
            glm::ivec4 value;
            std::memcpy(&value, data.data(), sizeof(glm::ivec4));
            return value;
        }

        bool MaterialParameterValue::GetBool() const {
            assert(type == Type::Bool && data.size() >= sizeof(bool));
            bool value;
            std::memcpy(&value, data.data(), sizeof(bool));
            return value;
        }

        glm::mat3 MaterialParameterValue::GetMat3() const {
            assert(type == Type::Mat3 && data.size() >= sizeof(glm::mat3));
            glm::mat3 value;
            std::memcpy(&value, data.data(), sizeof(glm::mat3));
            return value;
        }

        glm::mat4 MaterialParameterValue::GetMat4() const {
            assert(type == Type::Mat4 && data.size() >= sizeof(glm::mat4));
            glm::mat4 value;
            std::memcpy(&value, data.data(), sizeof(glm::mat4));
            return value;
        }

    } // namespace Resources
} // namespace FirstEngine
