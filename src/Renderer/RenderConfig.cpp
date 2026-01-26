#include "FirstEngine/Renderer/RenderConfig.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace FirstEngine {
    namespace Renderer {

        glm::mat4 CameraConfig::GetViewMatrix() const {
            return glm::lookAt(position, target, up);
        }

        glm::mat4 CameraConfig::GetProjectionMatrix(float aspectRatio) const {
            // Create perspective projection matrix using OpenGL-style perspective
            // Note: GLM's glm::perspective uses OpenGL conventions (Y up, Z range [-1, 1])
            // Vulkan uses different conventions (Y down, Z range [0, 1])
            // We need to flip Y axis for Vulkan clip space
            glm::mat4 proj = glm::perspective(
                glm::radians(fov),
                aspectRatio,
                nearPlane,
                farPlane
            );
            
            // Flip Y axis for Vulkan clip space
            // Vulkan's clip space has Y pointing down, while OpenGL has Y pointing up
            // This matrix flips Y and adjusts Z to [0, 1] range
            proj[1][1] *= -1.0f;
            
            return proj;
        }

        RenderConfig::RenderConfig() = default;
        RenderConfig::~RenderConfig() = default;

    } // namespace Renderer
} // namespace FirstEngine
