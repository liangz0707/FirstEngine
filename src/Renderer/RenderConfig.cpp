#include "FirstEngine/Renderer/RenderConfig.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace FirstEngine {
    namespace Renderer {

        glm::mat4 CameraConfig::GetViewMatrix() const {
            return glm::lookAt(position, target, up);
        }

        glm::mat4 CameraConfig::GetProjectionMatrix(float aspectRatio) const {
            return glm::perspective(
                glm::radians(fov),
                aspectRatio,
                nearPlane,
                farPlane
            );
        }

        RenderConfig::RenderConfig() = default;
        RenderConfig::~RenderConfig() = default;

    } // namespace Renderer
} // namespace FirstEngine
