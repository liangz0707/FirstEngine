#include "FirstEngine/Renderer/SceneRenderer.h"
#include <set>
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/Renderer/RenderParameterCollector.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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

            // Begin new frame for all ShadingMaterials - clears per-frame update tracking
            // This must be done before any FlushParametersToGPU calls to prevent
            // updating descriptor sets that are in use by command buffers
            std::set<ShadingMaterial*> processedMaterials;
            for (Resources::Entity* entity : visibleEntities) {
                if (!entity || !entity->IsActive()) {
                    continue;
                }
                const auto& components = entity->GetComponents();
                for (const auto& component : components) {
                    if (!component) {
                        continue;
                    }
                    auto* shadingMaterial = component->GetShadingMaterial();
                    if (shadingMaterial && processedMaterials.find(shadingMaterial) == processedMaterials.end()) {
                        shadingMaterial->BeginFrame();
                        processedMaterials.insert(shadingMaterial);
                    }
                }
            }
            

            std::vector<RenderItem> allItems;
            
            // Get camera matrices once for all entities (per-frame data)
            // These will be used for PerFrame uniform buffer
            glm::mat4 viewMatrix = m_CameraConfig.GetViewMatrix();
            glm::mat4 projMatrix = m_CameraConfig.GetProjectionMatrix(1.0f); // Aspect ratio will be updated per frame if needed
            glm::mat4 viewProjMatrix = projMatrix * viewMatrix;
            
            // Store camera matrices for use in EntityToRenderItems
            m_CachedViewMatrix = viewMatrix;
            m_CachedProjMatrix = projMatrix;
            m_CachedViewProjMatrix = viewProjMatrix;

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

                // Collect and apply render parameters per frame (before creating render item)
                // Get ShadingMaterial from component
                auto* shadingMaterial = component->GetShadingMaterial();
                if (shadingMaterial) {
                    // Create parameter collector and collect from various sources
                    RenderParameterCollector collector;
                    
                    // Collect from material resource (per-material data)
                    auto* materialResource = shadingMaterial->GetMaterialResource();
                    if (materialResource) {
                        collector.CollectFromMaterialResource(materialResource);
                    }
                    
                    // Collect from component (per-object data: modelMatrix, normalMatrix)
                    // Convert component to ModelComponent* for CollectFromComponent
                    auto* modelComponent = dynamic_cast<Resources::ModelComponent*>(component.get());
                    if (modelComponent) {
                        collector.CollectFromComponent(modelComponent, entity);
                    }
                    
                    // Collect from camera (per-frame data: viewMatrix, projectionMatrix, viewProjectionMatrix)
                    // Use cached matrices from BuildRenderQueueFromEntities
                    collector.CollectFromCamera(
                        Core::Mat4(m_CachedViewMatrix),
                        Core::Mat4(m_CachedProjMatrix),
                        Core::Mat4(m_CachedViewProjMatrix)
                    );
                    
                    // Apply all collected parameters to material
                    shadingMaterial->ApplyParameters(collector);
                    
                    // Update render parameters (applies to CPU-side uniform buffer data)
                    shadingMaterial->UpdateRenderParameters();
                    
                    // Flush parameters to GPU buffers (transfers CPU data to GPU)
                    shadingMaterial->FlushParametersToGPU(m_Device);
                }

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
                void* currentDescriptorSet0 = nullptr;
                void* currentDescriptorSet1 = nullptr;

                for (const auto& item : items) {
                    // Get pipeline from material data
                    RHI::IPipeline* itemPipeline = static_cast<RHI::IPipeline*>(item.materialData.pipeline);
                    void* itemDescriptorSet0 = item.materialData.descriptorSet; // Legacy: Set 0 from RenderItem
                    void* itemDescriptorSet1 = nullptr; // Set 1 (PerObject/PerFrame) - will be set from ShadingMaterial
                    
                    // If ShadingMaterial is available, prefer it for pipeline and descriptor set
                    if (item.materialData.shadingMaterial) {
                        auto* shadingMaterial = static_cast<ShadingMaterial*>(item.materialData.shadingMaterial);
                        if (shadingMaterial && shadingMaterial->IsCreated()) {
                            // Ensure pipeline is created (lazy creation)
                            // If renderPass is available, create pipeline now
                            if (renderPass && !shadingMaterial->GetShadingState().GetPipeline()) {
                                bool pipelineCreated = shadingMaterial->EnsurePipelineCreated(m_Device, renderPass);
                                if (!pipelineCreated) {
                                    std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: Failed to create pipeline for ShadingMaterial. "
                                              << "Device: " << (m_Device ? "valid" : "null") 
                                              << ", RenderPass: " << (renderPass ? "valid" : "null") << std::endl;
                                }
                            }
                            
                            itemPipeline = shadingMaterial->GetShadingState().GetPipeline();
                            // Get descriptor sets for Set 0 (MaterialParams/textures) and Set 1 (PerObject/PerFrame)
                            itemDescriptorSet0 = shadingMaterial->GetDescriptorSet(0);
                            itemDescriptorSet1 = shadingMaterial->GetDescriptorSet(1);
                            
                            // If pipeline is not created yet and we don't have renderPass, skip this item
                            if (!itemPipeline) {
                                if (!renderPass) {
                                    std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: itemPipeline is null and renderPass is null, skipping draw command" << std::endl;
                                } else {
                                    std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: itemPipeline is null after EnsurePipelineCreated, skipping draw command" << std::endl;
                                }
                                continue; // Skip this item if pipeline is not ready
                            }
                        } else if (shadingMaterial && !shadingMaterial->IsCreated()) {
                            std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: ShadingMaterial is not created yet, skipping draw command" << std::endl;
                            continue;
                        }
                    }
                    
                    // Bind pipeline if changed
                    // IMPORTANT: Always bind pipeline if itemPipeline is valid, even if it's the same as currentPipeline
                    // This ensures pipeline is bound before DrawIndexed, especially after render pass changes
                    if (!itemPipeline) {
                        // Pipeline is null - this should not happen if we skipped the item above
                        // But if it does, log a warning and skip this item
                        std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: itemPipeline is null, skipping draw command" << std::endl;
                        continue;
                    }
                    
                    if (itemPipeline != currentPipeline) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindPipeline;
                        cmd.params.bindPipeline.pipeline = itemPipeline;
                        commandList.AddCommand(std::move(cmd));
                        currentPipeline = itemPipeline;
                    }

                    // Bind descriptor sets if changed
                    // We need to bind both Set 0 (MaterialParams/textures) and Set 1 (PerObject/PerFrame)
                    // Check if either set has changed
                    if (itemDescriptorSet0 != currentDescriptorSet0 || itemDescriptorSet1 != currentDescriptorSet1) {
                        RenderCommand cmd;
                        cmd.type = RenderCommandType::BindDescriptorSets;
                        cmd.params.bindDescriptorSets.firstSet = 0; // Start from Set 0
                        cmd.params.bindDescriptorSets.descriptorSets.clear();
                        
                        // Add Set 0 (MaterialParams/textures) if available
                        if (itemDescriptorSet0) {
                            cmd.params.bindDescriptorSets.descriptorSets.push_back(itemDescriptorSet0);
                            // Mark descriptor set as in use to prevent updates while bound to command buffer
                            // Note: We need to get the MaterialDescriptorManager from the ShadingMaterial
                            // For now, we'll rely on the per-frame tracking in MaterialDescriptorManager
                        }
                        
                        // Add Set 1 (PerObject/PerFrame) if available
                        // Note: Vulkan requires consecutive descriptor sets without gaps
                        // If Set 0 exists, Set 1 can be added directly
                        // If Set 0 doesn't exist but Set 1 does, we need to handle this case
                        // For now, we'll only bind Set 1 if Set 0 also exists (to avoid gaps)
                        if (itemDescriptorSet1) {
                            // Only add Set 1 if Set 0 exists (to maintain consecutive sets)
                            // If Set 0 doesn't exist, Set 1 cannot be bound alone (Vulkan requirement)
                            if (itemDescriptorSet0) {
                                cmd.params.bindDescriptorSets.descriptorSets.push_back(itemDescriptorSet1);
                            } else {
                                std::cerr << "Warning: SceneRenderer::SubmitRenderQueue: Set 1 (PerObject/PerFrame) exists but Set 0 doesn't. "
                                          << "Cannot bind Set 1 alone (Vulkan requires consecutive sets). "
                                          << "This may indicate that PerObject/PerFrame uniform buffers are not being created correctly." << std::endl;
                            }
                        }
                        
                        // Only bind if we have at least one valid descriptor set
                        if (!cmd.params.bindDescriptorSets.descriptorSets.empty()) {
                            cmd.params.bindDescriptorSets.dynamicOffsets.clear();
                            commandList.AddCommand(std::move(cmd));
                            currentDescriptorSet0 = itemDescriptorSet0;
                            currentDescriptorSet1 = itemDescriptorSet1;
                        }
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
