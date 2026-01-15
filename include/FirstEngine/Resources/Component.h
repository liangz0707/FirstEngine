#pragma once

#include "FirstEngine/Resources/Export.h"
#include <cstdint>

namespace FirstEngine {
    namespace Resources {
        // Forward declarations
        class Entity;
        struct AABB;

        // Component types
        enum class ComponentType : uint32_t {
            Transform = 0,
            Mesh = 1,
            Light = 2,
            Effect = 3,
            Camera = 4,
            Collider = 5,
            Custom = 100
        };

        // Base component class
        class FE_RESOURCES_API Component {
        public:
            Component(ComponentType type);
            virtual ~Component() = default;

            ComponentType GetType() const { return m_Type; }
            Entity* GetEntity() const { return m_Entity; }
            void SetEntity(Entity* entity) { m_Entity = entity; }

            virtual void OnAttach() {}
            virtual void OnDetach() {}
            virtual AABB GetBounds() const;

        protected:
            ComponentType m_Type;
            Entity* m_Entity;
        };


    } // namespace Resources
} // namespace FirstEngine
