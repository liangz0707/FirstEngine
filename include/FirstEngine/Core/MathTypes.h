#pragma once

#include "FirstEngine/Core/Export.h"
#include <cstdint>
#include <cstring>

// Forward declaration - actual implementation can be swapped
// Currently uses GLM, but can be replaced with other math libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace FirstEngine {
    namespace Core {

        // ============================================================================
        // Math type aliases - abstract away the underlying math library
        // ============================================================================
        // These types can be easily swapped by changing the underlying implementation
        // Currently uses GLM, but can be replaced with DirectXMath, Eigen, etc.
        // ============================================================================

        // Vector types
        using Vec2 = glm::vec2;
        using Vec3 = glm::vec3;
        using Vec4 = glm::vec4;
        using IVec2 = glm::ivec2;
        using IVec3 = glm::ivec3;
        using IVec4 = glm::ivec4;

        // Matrix types
        using Mat3 = glm::mat3;
        using Mat4 = glm::mat4;

        // Quaternion (if needed)
        using Quat = glm::quat;

        // ============================================================================
        // Math utility functions
        // ============================================================================

        // Create identity matrix
        inline Mat4 Mat4Identity() {
            return glm::mat4(1.0f);
        }

        // Create zero matrix
        inline Mat4 Mat4Zero() {
            return glm::mat4(0.0f);
        }

        // Create translation matrix
        inline Mat4 Mat4Translation(const Vec3& translation) {
            return glm::translate(glm::mat4(1.0f), translation);
        }

        // Create rotation matrix (from quaternion)
        inline Mat4 Mat4Rotation(const Quat& rotation) {
            return glm::mat4_cast(rotation);
        }

        // Create scale matrix
        inline Mat4 Mat4Scale(const Vec3& scale) {
            return glm::scale(glm::mat4(1.0f), scale);
        }

        // Create perspective projection matrix
        inline Mat4 Mat4Perspective(float fov, float aspect, float nearPlane, float farPlane) {
            return glm::perspective(fov, aspect, nearPlane, farPlane);
        }

        // Create orthographic projection matrix
        inline Mat4 Mat4Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
            return glm::ortho(left, right, bottom, top, nearPlane, farPlane);
        }

        // Create look-at view matrix
        inline Mat4 Mat4LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
            return glm::lookAt(eye, center, up);
        }

        // Matrix multiplication
        inline Mat4 Mat4Multiply(const Mat4& a, const Mat4& b) {
            return a * b;
        }

        // Matrix inverse
        inline Mat4 Mat4Inverse(const Mat4& m) {
            return glm::inverse(m);
        }

        // Matrix transpose
        inline Mat4 Mat4Transpose(const Mat4& m) {
            return glm::transpose(m);
        }

        // Vector operations
        inline Vec3 Vec3Normalize(const Vec3& v) {
            return glm::normalize(v);
        }

        inline float Vec3Length(const Vec3& v) {
            return glm::length(v);
        }

        inline float Vec3Dot(const Vec3& a, const Vec3& b) {
            return glm::dot(a, b);
        }

        inline Vec3 Vec3Cross(const Vec3& a, const Vec3& b) {
            return glm::cross(a, b);
        }

        // Vector-Matrix multiplication
        inline Vec3 Vec3Transform(const Vec3& v, const Mat4& m) {
            return Vec3(m * Vec4(v, 1.0f));
        }

        inline Vec3 Vec3TransformNormal(const Vec3& v, const Mat4& m) {
            Mat3 normalMatrix = Mat3(Mat4Transpose(Mat4Inverse(m)));
            return normalMatrix * v;
        }

    } // namespace Core
} // namespace FirstEngine
