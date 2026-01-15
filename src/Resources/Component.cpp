#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Resources/Scene.h"

namespace FirstEngine {
    namespace Resources {

        Component::Component(ComponentType type)
            : m_Type(type), m_Entity(nullptr) {}

        AABB Component::GetBounds() const {
            return AABB(); // Default empty bounds
        }


    } // namespace Resources
} // namespace FirstEngine
