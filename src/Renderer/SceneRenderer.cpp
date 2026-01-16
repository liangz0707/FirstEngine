#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/ModelComponent.h"
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
            IRenderPass* pass,
            const RenderConfig& renderConfig,
            RHI::IRenderPass* renderPass
        ) {
            if (!scene) {
                m_SceneRenderCommands.Clear();
                return;
            }

            // Store current render pass for pipeline creation
            m_CurrentRenderPass = pass;

            // Determine camera config (use custom camera from pass if set, otherwise use global from RenderConfig)
            // This is done here to simplify FrameGraph::Execute
            if (pass && pass->UsesCustomCamera()) {
                m_CameraConfig = pass->GetCameraConfig();
            } else {
                m_CameraConfig = renderConfig.GetCamera();
            }

            // Clear previous commands
            m_SceneRenderCommands.Clear();

            // Get resolution config and render flags from RenderConfig
            const ResolutionConfig& resolutionConfig = renderConfig.GetResolution();
            const RenderFlags& renderFlags = renderConfig.GetRenderFlags();

            // Build render queue (uses stored camera config)
            RenderQueue renderQueue;
            BuildRenderQueue(scene, resolutionConfig, renderFlags, renderQueue);

            // Convert render queue to render command list
            // Pass renderPass to ensure pipelines are created
            m_SceneRenderCommands = SubmitRenderQueue(renderQueue, renderPass);
        }

        void SceneRenderer::BuildRenderQueue(
            Resources::Scene* scene,
            const ResolutionConfig& resolutionConfig,
            const RenderFlags& renderFlags,
            RenderQueue& renderQueue
        ) {
            if (!scene) {
                return;
            }

            renderQueue.Clear();

            // Get view and projection matrices from stored camera config
            glm::mat4 viewMatrix = m_CameraConfig.GetViewMatrix();
            glm::mat4 projMatrix = m_CameraConfig.GetProjectionMatrix(resolutionConfig.GetAspectRatio());
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

            // Build render items from visible entities
            // Component-level filtering is now done in CreateRenderItem
            BuildRenderQueueFromEntities(visibleEntities, renderQueue);

            // Update statistics
            size_t totalEntities = scene->GetAllEntities().size();
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
            // Entity now manages its own world matrix, no need to compute or pass it
            for (Resources::Entity* entity : visibleEntities) {
                if (!entity || !entity->IsActive()) {
                    continue;
                }

                // Entity's world matrix is cached and automatically updated
                EntityToRenderItems(entity, allItems);
            }

            // Add all items to render queue (will be batched automatically)
            for (const auto& item : allItems) {
                renderQueue.AddItem(item);
            }

            // Sort batches for optimal rendering
            renderQueue.Sort();
        }

        void SceneRenderer::EntityToRenderItems(
            Resources::Entity* entity,
            std::vector<RenderItem>& items
        ) {
            if (!entity || !entity->IsActive()) {
                return;
            }

            // Get world matrix from Entity (cached, automatically updated when dirty)
            // GetWorldMatrix() internally handles lazy update if matrix is dirty
            const glm::mat4& worldMatrix = entity->GetWorldMatrix();

            // Process all components that can render
            // Components now handle their own CreateRenderItem and MatchesRenderFlags
            const auto& components = entity->GetComponents();
            for (const auto& component : components) {
                if (!component) {
                    continue;
                }

                // Components are loaded via OnLoad() when Entity is fully loaded
                // No need to manually trigger loading here

                // Component creates its own render item (returns nullptr if doesn't match flags)
                auto renderItem = component->CreateRenderItem(worldMatrix, m_RenderFlags);
                if (renderItem) {
                    // Component matched render flags and created a valid render item
                    items.push_back(*renderItem);
                }
            }

            // Process children (children will compute their own world matrix from their parent)
            for (Resources::Entity* child : entity->GetChildren()) {
                EntityToRenderItems(child, items);
            }
        }

        // MatchesRenderFlags is now handled by Components themselves
        // No need for this method in SceneRenderer anymore

        RenderCommandList SceneRenderer::SubmitRenderQueue(const RenderQueue& renderQueue, RHI::IRenderPass* renderPass) {
            RenderCommandList commandList;

            // Convert render batches to render commands (data structures)
            const auto& batches = renderQueue.GetBatches();
            for (const auto& batch : batches) {
                const auto& items = batch.GetItems();

                RHI::IPipeline* currentPipeline = nullptr;
                void* currentDescriptorSet = nullptr;

                for (const auto& item : items) {
                    // Get pipeline from material data
                    RHI::IPipeline* itemPipeline = static_cast<RHI::IPipeline*>(item.materialData.pipeline);
                    void* itemDescriptorSet = item.materialData.descriptorSet;
                    
                    // If ShadingMaterial is available, prefer it for pipeline and descriptor set
                    if (item.materialData.shadingMaterial) {
                        auto* shadingMaterial = static_cast<ShadingMaterial*>(item.materialData.shadingMaterial);
                        if (shadingMaterial && shadingMaterial->IsCreated()) {
                            // Ensure pipeline is created (lazy creation)
                            // If renderPass is available, create pipeline now
                            if (renderPass && !shadingMaterial->GetShadingState().GetPipeline()) {
                                shadingMaterial->EnsurePipelineCreated(m_Device, renderPass);
                            }
                            
                            itemPipeline = shadingMaterial->GetShadingState().GetPipeline();
                            itemDescriptorSet = shadingMaterial->GetDescriptorSet(0);
                            
                            // If pipeline is not created yet and we don't have renderPass, skip this item
                            if (!itemPipeline) {
                                continue; // Skip this item if pipeline is not ready
                            }
                        }
                    }
                    
                    // Bind pipeline if changed
                    if (itemPipeline != currentPipeline) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindPipeline;
                        cmd.params.bindPipeline.pipeline = itemPipeline;
                        commandList.AddCommand(std::move(cmd));
                        currentPipeline = itemPipeline;
                    }

                    // Bind descriptor set if changed
                    if (itemDescriptorSet != currentDescriptorSet) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindDescriptorSets;
                        cmd.params.bindDescriptorSets.firstSet = 0;
                        if (itemDescriptorSet) {
                            cmd.params.bindDescriptorSets.descriptorSets = {itemDescriptorSet};
                        } else {
                            cmd.params.bindDescriptorSets.descriptorSets.clear();
                        }
                        cmd.params.bindDescriptorSets.dynamicOffsets.clear();
                        commandList.AddCommand(std::move(cmd));
                        currentDescriptorSet = itemDescriptorSet;
                    }

                    // Bind vertex buffers
                    if (item.geometryData.vertexBuffer) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindVertexBuffers;
                        cmd.params.bindVertexBuffers.firstBinding = 0;
                        cmd.params.bindVertexBuffers.buffers = {static_cast<RHI::IBuffer*>(item.geometryData.vertexBuffer)};
                        cmd.params.bindVertexBuffers.offsets = {item.geometryData.vertexBufferOffset};
                        commandList.AddCommand(std::move(cmd));
                    }

                    // Bind index buffer
                    if (item.geometryData.indexBuffer) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindIndexBuffer;
                        cmd.params.bindIndexBuffer.buffer = static_cast<RHI::IBuffer*>(item.geometryData.indexBuffer);
                        cmd.params.bindIndexBuffer.offset = item.geometryData.indexBufferOffset;
                        cmd.params.bindIndexBuffer.is32Bit = true;
                        commandList.AddCommand(std::move(cmd));
                    }

                    // Draw command
                    if (item.geometryData.indexCount > 0) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::DrawIndexed;
                        cmd.params.drawIndexed.indexCount = item.geometryData.indexCount;
                        cmd.params.drawIndexed.instanceCount = 1;
                        cmd.params.drawIndexed.firstIndex = item.geometryData.firstIndex;
                        cmd.params.drawIndexed.vertexOffset = 0;
                        cmd.params.drawIndexed.firstInstance = 0;
                        commandList.AddCommand(std::move(cmd));
                    } else if (item.geometryData.vertexCount > 0) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::Draw;
                        cmd.params.draw.vertexCount = item.geometryData.vertexCount;
                        cmd.params.draw.instanceCount = 1;
                        cmd.params.draw.firstVertex = item.geometryData.firstVertex;
                        cmd.params.draw.firstInstance = 0;
                        commandList.AddCommand(std::move(cmd));
                    }
                }
            }

            return commandList;
        }

        // CreateRenderItem is now handled by Components themselves
        // No need for this method in SceneRenderer anymore

    } // namespace Renderer
} // namespace FirstEngine
