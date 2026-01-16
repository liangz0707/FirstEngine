#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Renderer/RenderFlags.h"

namespace FirstEngine {
    namespace Resources {

        Component::Component(ComponentType type) : m_Type(type), m_Entity(nullptr) {}

        AABB Component::GetBounds() const {
            return AABB();
        }

        std::unique_ptr<Renderer::RenderItem> Component::CreateRenderItem(
            const glm::mat4& worldMatrix,
            Renderer::RenderObjectFlag renderFlags
        ) {
            // Default implementation returns nullptr
            // Subclasses should override to create render items from their resources
            return nullptr;
        }

        bool Component::MatchesRenderFlags(Renderer::RenderObjectFlag renderFlags) const {
            // Default implementation returns false
            // Subclasses should override to check if they match the render flags
            return false;
        }

    } // namespace Resources
} // namespace FirstEngine
