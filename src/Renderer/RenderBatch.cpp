#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Resources/Scene.h"
#include <algorithm>
#include <unordered_map>
#include <cmath>

namespace FirstEngine {
    namespace Renderer {

        // RenderBatch implementation
        RenderBatch::RenderBatch() = default;
        RenderBatch::~RenderBatch() = default;

        void RenderBatch::AddItem(const RenderItem& item) {
            m_Items.push_back(item);
        }

        void RenderBatch::Clear() {
            m_Items.clear();
        }

        void RenderBatch::Sort() {
            // Sort by pipeline, then by material, then by depth (front to back for opaque, back to front for transparent)
            std::sort(m_Items.begin(), m_Items.end(),
                [](const RenderItem& a, const RenderItem& b) {
                    // First sort by ShadingMaterial (if available)
                    void* aShadingMaterial = a.materialData.shadingMaterial;
                    void* bShadingMaterial = b.materialData.shadingMaterial;
                    if (aShadingMaterial != bShadingMaterial) {
                        if (aShadingMaterial && bShadingMaterial) {
                            // Both have materials, compare by pointer
                            return aShadingMaterial < bShadingMaterial;
                        }
                        // Items with materials come before items without
                        return aShadingMaterial != nullptr;
                    }
                    // Fallback to pipeline if ShadingMaterial is not available
                    if (a.materialData.pipeline != b.materialData.pipeline) {
                        return a.materialData.pipeline < b.materialData.pipeline;
                    }
                    // Then by descriptor set (material)
                    if (a.materialData.descriptorSet != b.materialData.descriptorSet) {
                        return a.materialData.descriptorSet < b.materialData.descriptorSet;
                    }
                    // Then by sort key (which includes depth)
                    return a.sortKey < b.sortKey;
                });
        }

        std::vector<RHI::IPipeline*> RenderBatch::GetUniquePipelines() const {
            std::vector<RHI::IPipeline*> pipelines;
            std::unordered_map<RHI::IPipeline*, bool> seen;

            for (const auto& item : m_Items) {
                RHI::IPipeline* pipeline = nullptr;
                
                // Prefer getting pipeline from ShadingMaterial if available
                if (item.materialData.shadingMaterial) {
                    auto* shadingMaterial = static_cast<ShadingMaterial*>(item.materialData.shadingMaterial);
                    if (shadingMaterial && shadingMaterial->IsCreated()) {
                        pipeline = shadingMaterial->GetShadingState().GetPipeline();
                    }
                }
                if (!pipeline && item.materialData.pipeline) {
                    // Fallback to direct pipeline pointer
                    pipeline = static_cast<RHI::IPipeline*>(item.materialData.pipeline);
                }
                
                if (pipeline && seen.find(pipeline) == seen.end()) {
                    pipelines.push_back(pipeline);
                    seen[pipeline] = true;
                }
            }

            return pipelines;
        }

        // RenderQueue implementation
        RenderQueue::RenderQueue() = default;
        RenderQueue::~RenderQueue() = default;

        void RenderQueue::AddItem(const RenderItem& item) {
            m_Items.push_back(item);
            m_NeedsRebuild = true;
        }

        void RenderQueue::Clear() {
            m_Items.clear();
            m_Batches.clear();
            m_NeedsRebuild = false;
        }

        void RenderQueue::Sort() {
            if (m_NeedsRebuild) {
                RebuildBatches();
            }

            for (auto& batch : m_Batches) {
                batch.Sort();
            }
        }

        size_t RenderQueue::GetTotalItemCount() const {
            size_t count = 0;
            for (const auto& batch : m_Batches) {
                count += batch.GetItemCount();
            }
            return count;
        }

        void RenderQueue::RebuildBatches() {
            m_Batches.clear();

            // Group items by pipeline and material
            std::unordered_map<uint64_t, RenderBatch> batchMap;

            for (const auto& item : m_Items) {
                // Create a key from ShadingMaterial (preferred) or pipeline and descriptor set pointers
                uint64_t key = 0;
                if (item.materialData.shadingMaterial) {
                    // Use ShadingMaterial pointer as primary key
                    key = reinterpret_cast<uint64_t>(item.materialData.shadingMaterial);
                } else {
                    // Fallback to pipeline and descriptor set
                    if (item.materialData.pipeline) {
                        key = reinterpret_cast<uint64_t>(item.materialData.pipeline);
                    }
                    if (item.materialData.descriptorSet) {
                        key ^= (reinterpret_cast<uint64_t>(item.materialData.descriptorSet) << 16);
                    }
                }

                batchMap[key].AddItem(item);
            }

            // Convert map to vector
            for (auto& pair : batchMap) {
                m_Batches.push_back(std::move(pair.second));
            }

            m_NeedsRebuild = false;
        }

        // Frustum implementation
        Frustum::Frustum(const glm::mat4& viewProjMatrix) {
            // Extract frustum planes from view-projection matrix
            // Left plane
            planes[0] = glm::vec4(
                viewProjMatrix[0][3] + viewProjMatrix[0][0],
                viewProjMatrix[1][3] + viewProjMatrix[1][0],
                viewProjMatrix[2][3] + viewProjMatrix[2][0],
                viewProjMatrix[3][3] + viewProjMatrix[3][0]
            );

            // Right plane
            planes[1] = glm::vec4(
                viewProjMatrix[0][3] - viewProjMatrix[0][0],
                viewProjMatrix[1][3] - viewProjMatrix[1][0],
                viewProjMatrix[2][3] - viewProjMatrix[2][0],
                viewProjMatrix[3][3] - viewProjMatrix[3][0]
            );

            // Bottom plane
            planes[2] = glm::vec4(
                viewProjMatrix[0][3] + viewProjMatrix[0][1],
                viewProjMatrix[1][3] + viewProjMatrix[1][1],
                viewProjMatrix[2][3] + viewProjMatrix[2][1],
                viewProjMatrix[3][3] + viewProjMatrix[3][1]
            );

            // Top plane
            planes[3] = glm::vec4(
                viewProjMatrix[0][3] - viewProjMatrix[0][1],
                viewProjMatrix[1][3] - viewProjMatrix[1][1],
                viewProjMatrix[2][3] - viewProjMatrix[2][1],
                viewProjMatrix[3][3] - viewProjMatrix[3][1]
            );

            // Near plane
            planes[4] = glm::vec4(
                viewProjMatrix[0][3] + viewProjMatrix[0][2],
                viewProjMatrix[1][3] + viewProjMatrix[1][2],
                viewProjMatrix[2][3] + viewProjMatrix[2][2],
                viewProjMatrix[3][3] + viewProjMatrix[3][2]
            );

            // Far plane
            planes[5] = glm::vec4(
                viewProjMatrix[0][3] - viewProjMatrix[0][2],
                viewProjMatrix[1][3] - viewProjMatrix[1][2],
                viewProjMatrix[2][3] - viewProjMatrix[2][2],
                viewProjMatrix[3][3] - viewProjMatrix[3][2]
            );

            // Normalize planes
            for (int i = 0; i < 6; ++i) {
                float length = glm::length(glm::vec3(planes[i]));
                if (length > 0.0f) {
                    planes[i] /= length;
                }
            }
        }

        bool Frustum::ContainsPoint(const glm::vec3& point) const {
            for (int i = 0; i < 6; ++i) {
                float distance = glm::dot(glm::vec3(planes[i]), point) + planes[i].w;
                if (distance < 0.0f) {
                    return false;
                }
            }
            return true;
        }

        bool Frustum::ContainsSphere(const glm::vec3& center, float radius) const {
            for (int i = 0; i < 6; ++i) {
                float distance = glm::dot(glm::vec3(planes[i]), center) + planes[i].w;
                if (distance < -radius) {
                    return false;
                }
            }
            return true;
        }

        bool Frustum::ContainsAABB(const Resources::AABB& aabb) const {
            // Test if AABB is outside any plane
            for (int i = 0; i < 6; ++i) {
                glm::vec3 normal = glm::vec3(this->planes[i]);
                float planeD = this->planes[i].w;

                // Find the vertex of AABB that is farthest in the negative direction of the plane normal
                glm::vec3 p;
                p.x = (normal.x > 0.0f) ? aabb.min.x : aabb.max.x;
                p.y = (normal.y > 0.0f) ? aabb.min.y : aabb.max.y;
                p.z = (normal.z > 0.0f) ? aabb.min.z : aabb.max.z;

                float distance = glm::dot(normal, p) + planeD;
                if (distance < 0.0f) {
                    return false; // AABB is outside this plane
                }
            }
            return true; // AABB intersects or is inside frustum
        }

        // CullingSystem implementation
        CullingSystem::CullingSystem() = default;
        CullingSystem::~CullingSystem() = default;

        void CullingSystem::CullEntities(
            const Frustum& frustum,
            const std::vector<Resources::Entity*>& entities,
            std::vector<Resources::Entity*>& visibleEntities
        ) const {
            visibleEntities.clear();
            visibleEntities.reserve(entities.size());

            for (Resources::Entity* entity : entities) {
                if (!entity || !entity->IsActive()) {
                    continue;
                }

                if (IsEntityVisible(frustum, entity)) {
                    visibleEntities.push_back(entity);
                }
            }
        }

        void CullingSystem::CullRenderItems(
            const Frustum& frustum,
            const std::vector<RenderItem>& items,
            std::vector<RenderItem>& visibleItems
        ) const {
            visibleItems.clear();
            visibleItems.reserve(items.size());

            for (const auto& item : items) {
                if (!item.entity) {
                    // If no entity, assume visible (could be UI or other non-spatial items)
                    visibleItems.push_back(item);
                    continue;
                }

                if (!item.entity->IsActive()) {
                    continue;
                }

                Resources::AABB bounds = item.entity->GetWorldBounds();
                if (frustum.ContainsAABB(bounds)) {
                    visibleItems.push_back(item);
                }
            }
        }

        void CullingSystem::PerformOcclusionCulling(
            const Frustum& frustum,
            const std::vector<Resources::Entity*>& entities,
            std::vector<Resources::Entity*>& visibleEntities
        ) const {
            // First do frustum culling
            CullEntities(frustum, entities, visibleEntities);

            // TODO: Implement GPU-based occlusion culling using occlusion queries
            // This would require:
            // 1. Render bounding boxes to depth buffer
            // 2. Use occlusion queries to test visibility
            // 3. Filter out occluded entities
        }

        bool CullingSystem::IsEntityVisible(const Frustum& frustum, Resources::Entity* entity) const {
            if (!entity) return false;

            Resources::AABB bounds = entity->GetWorldBounds();
            return frustum.ContainsAABB(bounds);
        }

    } // namespace Renderer
} // namespace FirstEngine
