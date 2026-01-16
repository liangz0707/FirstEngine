#include "FirstEngine/Resources/EffectComponent.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/Renderer/RenderBatch.h"

namespace FirstEngine {
    namespace Resources {

        EffectComponent::EffectComponent() : Component(ComponentType::Effect) {}

        bool EffectComponent::MatchesRenderFlags(Renderer::RenderObjectFlag renderFlags) const {
            if (!m_Enabled) {
                return false;
            }

            // Effect components typically render as transparent or UI elements
            // Check if UI or Transparent flag is set
            bool isUI = (renderFlags & Renderer::RenderObjectFlag::UI) != Renderer::RenderObjectFlag::None;
            bool isTransparent = (renderFlags & Renderer::RenderObjectFlag::Transparent) != Renderer::RenderObjectFlag::None;
            bool isAll = (renderFlags & Renderer::RenderObjectFlag::All) == Renderer::RenderObjectFlag::All;

            return isUI || isTransparent || isAll;
        }

        std::unique_ptr<Renderer::RenderItem> EffectComponent::CreateRenderItem(
            const glm::mat4& worldMatrix,
            Renderer::RenderObjectFlag renderFlags
        ) {
            // Check if this component matches render flags
            if (!MatchesRenderFlags(renderFlags)) {
                return nullptr; // Doesn't match, return nullptr
            }

            if (!m_Enabled || m_EffectPath.empty()) {
                return nullptr; // Effect not enabled or no effect path
            }

            // TODO: Implement effect rendering
            // For now, return nullptr as effect rendering is not yet implemented
            // This will be implemented when particle system is added
            return nullptr;
        }

    } // namespace Resources
} // namespace FirstEngine
