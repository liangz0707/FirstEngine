#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class Scene;
        class Entity;
    }
    namespace Renderer {
        class IRenderPass;
        class RenderConfig;
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

            // Set camera configuration (stored in SceneRenderer)
            // This allows each SceneRenderer to have its own camera (e.g., shadow pass)
            void SetCameraConfig(const CameraConfig& cameraConfig) { m_CameraConfig = cameraConfig; }
            const CameraConfig& GetCameraConfig() const { return m_CameraConfig; }
            CameraConfig& GetCameraConfig() { return m_CameraConfig; }

            // Render the scene (builds queue and generates commands)
            // Scene is passed as parameter (no longer stored in SceneRenderer)
            // Camera config is automatically determined from pass and renderConfig
            // renderPass: Optional RHI render pass for pipeline creation (if provided, pipelines will be created)
            // Returns the generated RenderCommandList which is stored internally
            void Render(
                Resources::Scene* scene,
                IRenderPass* pass,  // Pass that owns this SceneRenderer (for custom camera)
                const RenderConfig& renderConfig,  // Global render config (for default camera and other settings)
                RHI::IRenderPass* renderPass = nullptr  // RHI render pass for pipeline creation
            );

            // Get the generated render commands (stored internally after Render() call)
            const RenderCommandList& GetRenderCommands() const { return m_SceneRenderCommands; }
            RenderCommandList& GetRenderCommands() { return m_SceneRenderCommands; }

            // Check if this SceneRenderer needs to render (has valid commands)
            bool HasRenderCommands() const { return !m_SceneRenderCommands.IsEmpty(); }

            // Clear render commands
            void ClearRenderCommands() { m_SceneRenderCommands.Clear(); }

            // Convert render queue to render command list (data structure, no CommandBuffer dependency)
            // This method generates render commands as data, not GPU commands
            // renderPass: Optional render pass for pipeline creation (if nullptr, pipelines must be created elsewhere)
            RenderCommandList SubmitRenderQueue(const RenderQueue& renderQueue, RHI::IRenderPass* renderPass = nullptr);

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
            // Build render queue from scene (uses stored camera config)
            void BuildRenderQueue(
                Resources::Scene* scene,
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
            // Entity now caches its own world matrix, no need to pass parentTransform
            void EntityToRenderItems(
                Resources::Entity* entity,
                std::vector<RenderItem>& items
            );

            // Components now handle their own CreateRenderItem and MatchesRenderFlags
            // No need for these methods in SceneRenderer anymore

            // Convert render queue to render command list (internal helper)
            RenderCommandList SubmitRenderQueue(const RenderQueue& renderQueue);

            RHI::IDevice* m_Device;
            IRenderPass* m_CurrentRenderPass = nullptr; // Current render pass (set during Render())
            RenderObjectFlag m_RenderFlags = RenderObjectFlag::All;
            CameraConfig m_CameraConfig; // Camera configuration (stored in SceneRenderer)
            CullingSystem m_CullingSystem;
            bool m_FrustumCullingEnabled = true;
            bool m_OcclusionCullingEnabled = false;

            // Generated render commands (stored internally after Render() call)
            RenderCommandList m_SceneRenderCommands;

            // Statistics
            size_t m_VisibleEntityCount = 0;
            size_t m_CulledEntityCount = 0;
            size_t m_DrawCallCount = 0;
            
            // Cached camera matrices (computed once per frame in BuildRenderQueueFromEntities)
            glm::mat4 m_CachedViewMatrix = glm::mat4(1.0f);
            glm::mat4 m_CachedProjMatrix = glm::mat4(1.0f);
            glm::mat4 m_CachedViewProjMatrix = glm::mat4(1.0f);
        };

    } // namespace Renderer
} // namespace FirstEngine
