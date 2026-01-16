#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
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

        void SceneRenderer::Render(
            Resources::Scene* scene,
            const CameraConfig& cameraConfig,
            const ResolutionConfig& resolutionConfig,
            const RenderFlags& renderFlags
        ) {
            if (!scene) {
                m_SceneRenderCommands.Clear();
                return;
            }

            // Clear previous commands
            m_SceneRenderCommands.Clear();

            // Build render queue
            RenderQueue renderQueue;
            BuildRenderQueue(scene, cameraConfig, resolutionConfig, renderFlags, renderQueue);

            // Convert render queue to render command list
            m_SceneRenderCommands = SubmitRenderQueue(renderQueue);
        }

        void SceneRenderer::BuildRenderQueue(
            Resources::Scene* scene,
            const CameraConfig& cameraConfig,
            const ResolutionConfig& resolutionConfig,
            const RenderFlags& renderFlags,
            RenderQueue& renderQueue
        ) {
            if (!scene) {
                return;
            }

            renderQueue.Clear();

            // Get view and projection matrices from camera config
            glm::mat4 viewMatrix = cameraConfig.GetViewMatrix();
            glm::mat4 projMatrix = cameraConfig.GetProjectionMatrix(resolutionConfig.GetAspectRatio());
            glm::mat4 viewProjMatrix = projMatrix * viewMatrix;
            Frustum frustum(viewProjMatrix);

            // Use render flags from parameter
            bool frustumCulling = renderFlags.frustumCulling && m_FrustumCullingEnabled;
            bool occlusionCulling = renderFlags.occlusionCulling && m_OcclusionCullingEnabled;

            // Get visible entities
            std::vector<Resources::Entity*> visibleEntities;

            if (frustumCulling) {
                // Use octree for efficient culling
                visibleEntities = scene->QueryFrustum(viewProjMatrix);
                
                // Additional culling pass (optional, for more precise culling)
                if (occlusionCulling) {
                    m_CullingSystem.PerformOcclusionCulling(frustum, visibleEntities, visibleEntities);
                }
            } else {
                // No culling - get all active entities from all levels
                std::vector<Resources::Entity*> allEntities = scene->GetAllEntities();
                for (Resources::Entity* entity : allEntities) {
                    if (entity && entity->IsActive()) {
                        visibleEntities.push_back(entity);
                    }
                }
            }

            // Filter entities by render flags
            std::vector<Resources::Entity*> filteredEntities;
            for (Resources::Entity* entity : visibleEntities) {
                if (MatchesRenderFlags(entity)) {
                    filteredEntities.push_back(entity);
                }
            }

            // Build render items from filtered entities
            BuildRenderQueueFromEntities(filteredEntities, renderQueue);

            // Update statistics
            size_t totalEntities = scene->GetAllEntities().size();
            m_VisibleEntityCount = filteredEntities.size();
            m_CulledEntityCount = totalEntities - filteredEntities.size();
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

        bool SceneRenderer::MatchesRenderFlags(Resources::Entity* entity) const {
            if (!entity) {
                return false;
            }

            // TODO: Check entity's render flags against m_RenderFlags
            // For now, if RenderObjectFlag::All is set, accept all entities
            if ((m_RenderFlags & RenderObjectFlag::All) == RenderObjectFlag::All) {
                return true;
            }

            // TODO: Implement proper flag checking based on entity properties
            // This would check if entity has Shadow/Transparent/Opaque/UI flags
            // For now, default to accepting all entities
            return true;
        }

        RenderCommandList SceneRenderer::SubmitRenderQueue(const RenderQueue& renderQueue) {
            RenderCommandList commandList;

            // Convert render batches to render commands (data structures)
            const auto& batches = renderQueue.GetBatches();
            for (const auto& batch : batches) {
                const auto& items = batch.GetItems();

                RHI::IPipeline* currentPipeline = nullptr;
                void* currentDescriptorSet = nullptr;

                for (const auto& item : items) {
                    // Bind pipeline if changed
                    if (item.pipeline != currentPipeline) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindPipeline;
                        cmd.params.bindPipeline.pipeline = item.pipeline;
                        commandList.AddCommand(std::move(cmd));
                        currentPipeline = item.pipeline;
                    }

                    // Bind descriptor set if changed
                    if (item.descriptorSet != currentDescriptorSet) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindDescriptorSets;
                        cmd.params.bindDescriptorSets.firstSet = 0;
                        if (item.descriptorSet) {
                            cmd.params.bindDescriptorSets.descriptorSets = {item.descriptorSet};
                        } else {
                            cmd.params.bindDescriptorSets.descriptorSets.clear();
                        }
                        cmd.params.bindDescriptorSets.dynamicOffsets.clear();
                        commandList.AddCommand(std::move(cmd));
                        currentDescriptorSet = item.descriptorSet;
                    }

                    // Bind vertex buffers
                    if (item.vertexBuffer) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindVertexBuffers;
                        cmd.params.bindVertexBuffers.firstBinding = 0;
                        cmd.params.bindVertexBuffers.buffers = {item.vertexBuffer};
                        cmd.params.bindVertexBuffers.offsets = {item.vertexBufferOffset};
                        commandList.AddCommand(std::move(cmd));
                    }

                    // Bind index buffer
                    if (item.indexBuffer) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindIndexBuffer;
                        cmd.params.bindIndexBuffer.buffer = item.indexBuffer;
                        cmd.params.bindIndexBuffer.offset = item.indexBufferOffset;
                        cmd.params.bindIndexBuffer.is32Bit = true;
                        commandList.AddCommand(std::move(cmd));
                    }

                    // Draw command
                    if (item.indexCount > 0) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::DrawIndexed;
                        cmd.params.drawIndexed.indexCount = item.indexCount;
                        cmd.params.drawIndexed.instanceCount = 1;
                        cmd.params.drawIndexed.firstIndex = item.firstIndex;
                        cmd.params.drawIndexed.vertexOffset = 0;
                        cmd.params.drawIndexed.firstInstance = 0;
                        commandList.AddCommand(std::move(cmd));
                    } else if (item.vertexCount > 0) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::Draw;
                        cmd.params.draw.vertexCount = item.vertexCount;
                        cmd.params.draw.instanceCount = 1;
                        cmd.params.draw.firstVertex = item.firstVertex;
                        cmd.params.draw.firstInstance = 0;
                        commandList.AddCommand(std::move(cmd));
                    }
                }
            }

            return commandList;
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
