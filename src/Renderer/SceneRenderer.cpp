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
            : IRenderer(device) {}

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
            m_SceneRenderCommands = IRenderer::SubmitRenderQueue(renderQueue, renderPass);
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
                    
                    // FlushParametersToGPU now handles all parameters (including textures) in a single pass
                    // This applies parameters to CPU-side data and flushes to GPU buffers
                    // This avoids duplicate processing that occurred when UpdateRenderParameters() was called separately
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


        // CreateRenderItem is now handled by Components themselves
        // No need for this method in SceneRenderer anymore

    } // namespace Renderer
} // namespace FirstEngine
