#include "FirstEngine/Resources/CameraComponent.h"
#include "FirstEngine/Resources/Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace FirstEngine {
    namespace Resources {

        CameraComponent::CameraComponent() : Component(ComponentType::Camera) {}

        glm::mat4 CameraComponent::GetViewMatrix() const {
            if (!m_Entity) {
                return glm::mat4(1.0f);
            }

            const Transform& transform = m_Entity->GetTransform();
            glm::vec3 position = transform.position;
            glm::vec3 forward = transform.GetForward();
            glm::vec3 up = transform.GetUp();

            // Create view matrix (look-at matrix)
            return glm::lookAt(position, position + forward, up);
        }

        glm::mat4 CameraComponent::GetProjectionMatrix(float aspectRatio) const {
            // Convert FOV from degrees to radians
            float fovRadians = glm::radians(m_FOV);
            
            // Create perspective projection matrix
            return glm::perspective(fovRadians, aspectRatio, m_Near, m_Far);
        }

        glm::mat4 CameraComponent::GetViewProjectionMatrix(float aspectRatio) const {
            return GetProjectionMatrix(aspectRatio) * GetViewMatrix();
        }

    } // namespace Resources
} // namespace FirstEngine
