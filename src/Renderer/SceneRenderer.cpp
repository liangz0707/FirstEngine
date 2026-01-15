#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Renderer/RenderResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <algorithm>
#include <cmath>

namespace FirstEngine {
    namespace Renderer {

        SceneRenderer::SceneRenderer(RHI::IDevice* device)
            : m_Device(device) {}

        SceneRenderer::~SceneRenderer() = default;

        void SceneRenderer::SetScene(Resources::Scene* scene) {
            m_Scene = scene;
        }

        void SceneRenderer::BuildRenderQueue(
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix,
            RenderQueue& renderQueue
        ) {
            if (!m_Scene) {
                return;
            }

            renderQueue.Clear();

            glm::mat4 viewProjMatrix = projMatrix * viewMatrix;
            Frustum frustum(viewProjMatrix);

            // Get visible entities
            std::vector<Resources::Entity*> visibleEntities;

            if (m_FrustumCullingEnabled) {
                // Use octree for efficient culling
                visibleEntities = m_Scene->QueryFrustum(viewProjMatrix);
                
                // Additional culling pass (optional, for more precise culling)
                if (m_OcclusionCullingEnabled) {
                    m_CullingSystem.PerformOcclusionCulling(frustum, visibleEntities, visibleEntities);
                }
            } else {
                // No culling - get all active entities from all levels
                std::vector<Resources::Entity*> allEntities = m_Scene->GetAllEntities();
                for (Resources::Entity* entity : allEntities) {
                    if (entity && entity->IsActive()) {
                        visibleEntities.push_back(entity);
                    }
                }
            }

            // Build render items from visible entities
            BuildRenderQueueFromEntities(visibleEntities, renderQueue);

            // Update statistics
            size_t totalEntities = m_Scene->GetAllEntities().size();
            m_VisibleEntityCount = visibleEntities.size();
            m_CulledEntityCount = totalEntities - visibleEntities.size();
            m_DrawCallCount = renderQueue.GetTotalItemCount();
        }

        void SceneRenderer::BuildRenderQueueFromEntities(
            const std::vector<Resources::Entity*>& visibleEntities,
            RenderQueue& renderQueue
        ) {
            renderQueue.Clear();

            std::vector<RenderItem> allItems;

            // Convert entities to render items
            for (Resources::Entity* entity : visibleEntities) {
                if (!entity || !entity->IsActive()) {
                    continue;
                }

                glm::mat4 worldMatrix = entity->GetTransform().GetMatrix();
                EntityToRenderItems(entity, worldMatrix, allItems);
            }

            // Add all items to render queue (will be batched automatically)
            for (const auto& item : allItems) {
                renderQueue.AddItem(item);
            }

            // Sort batches for optimal rendering
            renderQueue.Sort();
        }

        void SceneRenderer::Render(
            RHI::ICommandBuffer* commandBuffer,
            const glm::mat4& viewMatrix,
            const glm::mat4& projMatrix
        ) {
            if (!commandBuffer || !m_Scene) {
                return;
            }

            RenderQueue renderQueue;
            BuildRenderQueue(viewMatrix, projMatrix, renderQueue);

            // Execute render batches
            const auto& batches = renderQueue.GetBatches();
            for (const auto& batch : batches) {
                const auto& items = batch.GetItems();

                RHI::IPipeline* currentPipeline = nullptr;
                void* currentDescriptorSet = nullptr;

                for (const auto& item : items) {
                    // Bind pipeline if changed
                    if (item.pipeline != currentPipeline) {
                        commandBuffer->BindPipeline(item.pipeline);
                        currentPipeline = item.pipeline;
                    }

                    // Bind descriptor set if changed
                    if (item.descriptorSet != currentDescriptorSet) {
                        // TODO: Implement descriptor set binding
                        // commandBuffer->BindDescriptorSets(0, {item.descriptorSet}, {});
                        currentDescriptorSet = item.descriptorSet;
                    }

                    // Bind vertex and index buffers
                    if (item.vertexBuffer) {
                        commandBuffer->BindVertexBuffers(0, {item.vertexBuffer}, {item.vertexBufferOffset});
                    }
                    if (item.indexBuffer) {
                        commandBuffer->BindIndexBuffer(item.indexBuffer, item.indexBufferOffset, true);
                    }

                    // Draw
                    if (item.indexCount > 0) {
                        commandBuffer->DrawIndexed(item.indexCount, 1, item.firstIndex, 0);
                    } else if (item.vertexCount > 0) {
                        commandBuffer->Draw(item.vertexCount, 1, item.firstVertex, 0);
                    }
                }
            }
        }

        void SceneRenderer::EntityToRenderItems(
            Resources::Entity* entity,
            const glm::mat4& parentTransform,
            std::vector<RenderItem>& items
        ) {
            if (!entity || !entity->IsActive()) {
                return;
            }

            // Compute world transform
            glm::mat4 worldMatrix = parentTransform * entity->GetTransform().GetMatrix();
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));

            // Process model components
            auto* modelComponent = entity->GetComponent<Resources::ModelComponent>();
            if (modelComponent) {
                RenderItem item = CreateRenderItem(modelComponent, entity, worldMatrix);
                item.normalMatrix = normalMatrix;
                items.push_back(item);
            }

            // Process children
            for (Resources::Entity* child : entity->GetChildren()) {
                EntityToRenderItems(child, worldMatrix, items);
            }
        }

        RenderItem SceneRenderer::CreateRenderItem(
            Resources::ModelComponent* modelComponent,
            Resources::Entity* entity,
            const glm::mat4& worldMatrix
        ) {
            RenderItem item;
            item.entity = entity;
            item.worldMatrix = worldMatrix;
            
            // Get model from component
            auto* model = modelComponent->GetModel();
            if (!model || model->GetMeshCount() == 0) {
                return item; // Return empty item if no model
            }

            // Get material from model (first mesh's material)
            Resources::MaterialHandle materialResource = model->GetMaterial(0);
            if (materialResource) {
                item.materialName = materialResource->GetMetadata().name;
            } else {
                item.materialName = "Default";
            }

            // Get mesh from model and create render mesh
            auto* mesh = model->GetMesh(0);
            if (mesh) {
                // Create render mesh directly (or use cached render mesh)
                // For now, create render mesh on the fly
                auto renderMesh = std::make_unique<Renderer::RenderMesh>();
                if (renderMesh->Initialize(m_Device, mesh)) {
                    item.vertexBuffer = renderMesh->GetVertexBuffer();
                    item.indexBuffer = renderMesh->GetIndexBuffer();
                    item.vertexCount = renderMesh->GetVertexCount();
                    item.indexCount = renderMesh->GetIndexCount();
                    item.firstIndex = 0;
                    item.firstVertex = 0;
                    
                    // Cache render mesh for reuse (optional - could be managed elsewhere)
                    // For now, render mesh is created per frame - could be optimized with caching
                }
            }

            // Compute sort key (pipeline ID, material ID, depth)
            // This is a simplified version - real implementation would use actual IDs
            uint64_t pipelineID = reinterpret_cast<uint64_t>(item.pipeline) & 0xFFFF;
            uint64_t materialID = std::hash<std::string>{}(item.materialName) & 0xFFFF;
            float depth = worldMatrix[3][2]; // Z component of translation
            uint64_t depthKey = static_cast<uint64_t>((depth + 1000.0f) * 1000.0f) & 0xFFFFFFFF;
            
            item.sortKey = (pipelineID << 48) | (materialID << 32) | depthKey;

            return item;
        }

    } // namespace Renderer
} // namespace FirstEngine
