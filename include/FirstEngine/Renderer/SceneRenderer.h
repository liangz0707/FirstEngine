#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/RHI/IDevice.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class Scene;
        class Entity;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Scene renderer - converts scene data to render commands
        // SceneRenderer is owned by IRenderPass and does not directly hold Scene reference
        // Scene is passed during Render() call
        class FE_RENDERER_API SceneRenderer {
        public:
            SceneRenderer(RHI::IDevice* device);
            ~SceneRenderer();

            // Set render object flags - determines which objects to render
            void SetRenderFlags(RenderObjectFlag flags) { m_RenderFlags = flags; }
            RenderObjectFlag GetRenderFlags() const { return m_RenderFlags; }

            // Render the scene (builds queue and generates commands)
            // Scene is passed as parameter (no longer stored in SceneRenderer)
            // Camera config can be different from global RenderConfig (e.g., for shadow pass)
            // Returns the generated RenderCommandList which is stored internally
            void Render(
                Resources::Scene* scene,
                const CameraConfig& cameraConfig,
                const ResolutionConfig& resolutionConfig,
                const RenderFlags& renderFlags
            );

            // Get the generated render commands (stored internally after Render() call)
            const RenderCommandList& GetRenderCommands() const { return m_SceneRenderCommands; }
            RenderCommandList& GetRenderCommands() { return m_SceneRenderCommands; }

            // Check if this SceneRenderer needs to render (has valid commands)
            bool HasRenderCommands() const { return !m_SceneRenderCommands.IsEmpty(); }

            // Clear render commands
            void ClearRenderCommands() { m_SceneRenderCommands.Clear(); }

            // Enable/disable features
            void SetFrustumCullingEnabled(bool enabled) { m_FrustumCullingEnabled = enabled; }
            bool IsFrustumCullingEnabled() const { return m_FrustumCullingEnabled; }

            void SetOcclusionCullingEnabled(bool enabled) { m_OcclusionCullingEnabled = enabled; }
            bool IsOcclusionCullingEnabled() const { return m_OcclusionCullingEnabled; }

            // Get statistics
            size_t GetVisibleEntityCount() const { return m_VisibleEntityCount; }
            size_t GetCulledEntityCount() const { return m_CulledEntityCount; }
            size_t GetDrawCallCount() const { return m_DrawCallCount; }

        private:
            // Build render queue from scene
            void BuildRenderQueue(
                Resources::Scene* scene,
                const CameraConfig& cameraConfig,
                const ResolutionConfig& resolutionConfig,
                const RenderFlags& renderFlags,
                RenderQueue& renderQueue
            );

            // Build render queue from visible entities (after culling)
            void BuildRenderQueueFromEntities(
                const std::vector<Resources::Entity*>& visibleEntities,
                RenderQueue& renderQueue
            );

            // Convert entity to render items (filtered by render flags)
            void EntityToRenderItems(
                Resources::Entity* entity,
                const glm::mat4& parentTransform,
                std::vector<RenderItem>& items
            );

            // Create render item from model component
            RenderItem CreateRenderItem(
                Resources::ModelComponent* modelComponent,
                Resources::Entity* entity,
                const glm::mat4& worldMatrix
            );

            // Check if entity matches render flags
            bool MatchesRenderFlags(Resources::Entity* entity) const;

            RHI::IDevice* m_Device;
            RenderObjectFlag m_RenderFlags = RenderObjectFlag::All;
            CullingSystem m_CullingSystem;
            bool m_FrustumCullingEnabled = true;
            bool m_OcclusionCullingEnabled = false;

            // Generated render commands (stored internally after Render() call)
            RenderCommandList m_SceneRenderCommands;

            // Statistics
            size_t m_VisibleEntityCount = 0;
            size_t m_CulledEntityCount = 0;
            size_t m_DrawCallCount = 0;
        };

    } // namespace Renderer
} // namespace FirstEngine
