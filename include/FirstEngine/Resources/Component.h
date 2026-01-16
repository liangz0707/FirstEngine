#pragma once

#include "FirstEngine/Resources/Export.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
namespace FirstEngine {
    namespace Renderer {
        struct RenderItem;
        enum class RenderObjectFlag : uint32_t;
    }
}

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
            virtual void OnLoad() {} // Called when Entity is fully loaded (all components attached, resources ready)
            virtual AABB GetBounds() const;

            // Render item creation - components that can render should override this
            // Returns nullptr if component doesn't render or doesn't match render flags
            // worldMatrix: The world transformation matrix for this entity
            // renderFlags: The render flags to check against
            virtual std::unique_ptr<Renderer::RenderItem> CreateRenderItem(
                const glm::mat4& worldMatrix,
                Renderer::RenderObjectFlag renderFlags
            );

            // Check if this component matches the render flags
            // Returns true if the component should be rendered with the given flags
            virtual bool MatchesRenderFlags(Renderer::RenderObjectFlag renderFlags) const;

        protected:
            ComponentType m_Type;
            Entity* m_Entity;
        };


    } // namespace Resources
} // namespace FirstEngine
