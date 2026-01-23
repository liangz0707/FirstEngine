#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        // Camera component
        class FE_RESOURCES_API CameraComponent : public Component {
        public:
            CameraComponent();
            ~CameraComponent() override = default;

            // Camera parameters
            void SetFOV(float fov) { m_FOV = fov; }
            float GetFOV() const { return m_FOV; }

            void SetNear(float nearPlane) { m_Near = nearPlane; }
            float GetNear() const { return m_Near; }

            void SetFar(float farPlane) { m_Far = farPlane; }
            float GetFar() const { return m_Far; }

            // Main camera flag
            void SetIsMainCamera(bool isMain) { m_IsMainCamera = isMain; }
            bool IsMainCamera() const { return m_IsMainCamera; }

            // Get view matrix (from entity transform)
            glm::mat4 GetViewMatrix() const;

            // Get projection matrix
            glm::mat4 GetProjectionMatrix(float aspectRatio) const;

            // Get view-projection matrix
            glm::mat4 GetViewProjectionMatrix(float aspectRatio) const;

        private:
            float m_FOV = 60.0f; // Field of view in degrees
            float m_Near = 0.1f;
            float m_Far = 1000.0f;
            bool m_IsMainCamera = false;
        };

    } // namespace Resources
} // namespace FirstEngine
