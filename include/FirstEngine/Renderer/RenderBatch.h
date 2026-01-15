#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IPipeline.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include "FirstEngine/Resources/Scene.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {
        class ModelComponent;
        class Entity;
    }

    namespace Renderer {

        // Render item represents a single draw call
        struct FE_RENDERER_API RenderItem {
            // Geometry
            RHI::IBuffer* vertexBuffer = nullptr;
            RHI::IBuffer* indexBuffer = nullptr;
            uint32_t indexCount = 0;
            uint32_t vertexCount = 0;
            uint32_t firstIndex = 0;
            uint32_t firstVertex = 0;
            uint64_t vertexBufferOffset = 0;
            uint64_t indexBufferOffset = 0;

            // Pipeline state
            RHI::IPipeline* pipeline = nullptr;
            void* descriptorSet = nullptr; // Material/descriptor set

            // Transform
            glm::mat4 worldMatrix = glm::mat4(1.0f);
            glm::mat4 normalMatrix = glm::mat4(1.0f);

            // Material properties
            std::string materialName;
            RHI::IImage* albedoTexture = nullptr;
            RHI::IImage* normalTexture = nullptr;
            RHI::IImage* metallicRoughnessTexture = nullptr;
            RHI::IImage* emissiveTexture = nullptr;

            // Sorting key (for batching)
            uint64_t sortKey = 0;

            // Entity reference (optional, for per-object data)
            Resources::Entity* entity = nullptr;
        };

        // Render batch groups render items by pipeline/material for efficient rendering
        class FE_RENDERER_API RenderBatch {
        public:
            RenderBatch();
            ~RenderBatch();

            void AddItem(const RenderItem& item);
            void Clear();
            size_t GetItemCount() const { return m_Items.size(); }
            const std::vector<RenderItem>& GetItems() const { return m_Items; }

            // Sort items for optimal rendering (by pipeline, then by material, then by depth)
            void Sort();

            // Get unique pipelines used in this batch
            std::vector<RHI::IPipeline*> GetUniquePipelines() const;

        private:
            std::vector<RenderItem> m_Items;
        };

        // Render queue manages all render batches for a frame
        class FE_RENDERER_API RenderQueue {
        public:
            RenderQueue();
            ~RenderQueue();

            // Add render item (automatically batched)
            void AddItem(const RenderItem& item);

            // Get batches sorted by render order
            const std::vector<RenderBatch>& GetBatches() const { return m_Batches; }

            // Clear all items
            void Clear();

            // Sort all batches
            void Sort();

            // Statistics
            size_t GetTotalItemCount() const;
            size_t GetBatchCount() const { return m_Batches.size(); }

        private:
            // Group items by pipeline and material
            void RebuildBatches();

            std::vector<RenderItem> m_Items;
            std::vector<RenderBatch> m_Batches;
            bool m_NeedsRebuild = true;
        };

        // Frustum culling helper
        struct FE_RENDERER_API Frustum {
            // Frustum planes (normalized)
            glm::vec4 planes[6]; // left, right, bottom, top, near, far

            Frustum() = default;
            Frustum(const glm::mat4& viewProjMatrix);

            bool ContainsPoint(const glm::vec3& point) const;
            bool ContainsSphere(const glm::vec3& center, float radius) const;
            bool ContainsAABB(const Resources::AABB& aabb) const;
        };

        // Culling system
        class FE_RENDERER_API CullingSystem {
        public:
            CullingSystem();
            ~CullingSystem();

            // Perform frustum culling on entities
            void CullEntities(
                const Frustum& frustum,
                const std::vector<Resources::Entity*>& entities,
                std::vector<Resources::Entity*>& visibleEntities
            ) const;

            // Perform frustum culling on render items
            void CullRenderItems(
                const Frustum& frustum,
                const std::vector<RenderItem>& items,
                std::vector<RenderItem>& visibleItems
            ) const;

            // Occlusion culling (optional, requires GPU queries)
            void PerformOcclusionCulling(
                const Frustum& frustum,
                const std::vector<Resources::Entity*>& entities,
                std::vector<Resources::Entity*>& visibleEntities
            ) const;

        private:
            bool IsEntityVisible(const Frustum& frustum, Resources::Entity* entity) const;
        };

    } // namespace Renderer
} // namespace FirstEngine
