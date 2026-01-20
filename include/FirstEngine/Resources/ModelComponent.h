#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include <string>
#include <memory>
#include <vector>

// Forward declarations
namespace FirstEngine {
    namespace RHI {
        class IDevice;
    }
    namespace Renderer {
        struct RenderItem;
        enum class RenderObjectFlag : uint32_t;
    }
}

namespace FirstEngine {
    namespace Resources {

        // Model component - references a model resource and provides render data
        // IRenderResources are stored in MeshResource and MaterialResource handles, not in Component
        class FE_RESOURCES_API ModelComponent : public Component {
        public:
            ModelComponent();
            ~ModelComponent() override;

            // Resource management
            void SetModel(ModelHandle model);
            ModelHandle GetModel() const { return m_Model; }

            // Bounds
            AABB GetBounds() const override;

            // Component interface - render item creation
            std::unique_ptr<Renderer::RenderItem> CreateRenderItem(
                const glm::mat4& worldMatrix,
                Renderer::RenderObjectFlag renderFlags
            ) override;

            // Component interface - render flags matching
            bool MatchesRenderFlags(Renderer::RenderObjectFlag renderFlags) const override;

            // Component::OnLoad override
            // Called when Entity is fully loaded (all components attached, resources ready)
            // Creates RenderGeometry in MeshResource handles and ShadingMaterial in MaterialResource handles
            void OnLoad() override;

            // Component::GetShadingMaterial override
            // Returns the ShadingMaterial for this component (from MaterialResource)
            Renderer::ShadingMaterial* GetShadingMaterial() const override;

        private:
            ModelHandle m_Model = nullptr;
        };

    } // namespace Resources
} // namespace FirstEngine
