#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/Resources/Scene.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // Scene renderer - converts scene data to render commands
        class FE_RENDERER_API SceneRenderer {
        public:
            SceneRenderer(RHI::IDevice* device);
            ~SceneRenderer();

            // Set the scene to render
            void SetScene(Resources::Scene* scene);
            Resources::Scene* GetScene() const { return m_Scene; }

            // Build render queue from scene (with frustum culling)
            void BuildRenderQueue(
                const glm::mat4& viewMatrix,
                const glm::mat4& projMatrix,
                RenderQueue& renderQueue
            );

            // Build render queue from visible entities (after culling)
            void BuildRenderQueueFromEntities(
                const std::vector<Resources::Entity*>& visibleEntities,
                RenderQueue& renderQueue
            );

            // Render the scene (builds queue and executes)
            void Render(
                RHI::ICommandBuffer* commandBuffer,
                const glm::mat4& viewMatrix,
                const glm::mat4& projMatrix
            );

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
            // Convert entity to render items
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

            RHI::IDevice* m_Device;
            Resources::Scene* m_Scene = nullptr;
            CullingSystem m_CullingSystem;
            bool m_FrustumCullingEnabled = true;
            bool m_OcclusionCullingEnabled = false;

            // Statistics
            size_t m_VisibleEntityCount = 0;
            size_t m_CulledEntityCount = 0;
            size_t m_DrawCallCount = 0;
        };

    } // namespace Renderer
} // namespace FirstEngine
