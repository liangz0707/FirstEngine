#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Resources/SceneLevel.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <array>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        class Scene;
        class Entity;
        class Component;
        class SceneLevel;

        // Transform component (always present on entities)
        struct FE_RESOURCES_API Transform {
            glm::vec3 position = glm::vec3(0.0f);
            glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // w, x, y, z
            glm::vec3 scale = glm::vec3(1.0f);

            glm::mat4 GetMatrix() const;
            glm::vec3 GetForward() const;
            glm::vec3 GetRight() const;
            glm::vec3 GetUp() const;
        };

        // Bounding box for spatial queries
        struct FE_RESOURCES_API AABB {
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);

            AABB() = default;
            AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

            glm::vec3 GetCenter() const { return (min + max) * 0.5f; }
            glm::vec3 GetSize() const { return max - min; }
            glm::vec3 GetHalfSize() const { return GetSize() * 0.5f; }
            bool Contains(const glm::vec3& point) const;
            bool Intersects(const AABB& other) const;
            AABB Transform(const glm::mat4& transform) const;
        };

        // Forward declarations for components (defined in separate files)
        class LightComponent;
        class EffectComponent;

        // Entity class
        class FE_RESOURCES_API Entity {
        public:
            Entity(Scene* scene, uint64_t id, const std::string& name = "");
            ~Entity();

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            Entity(const Entity&) = delete;
            Entity& operator=(const Entity&) = delete;

            // Allow move constructor and move assignment operator
            Entity(Entity&&) noexcept = default;
            Entity& operator=(Entity&&) noexcept = default;

            uint64_t GetID() const { return m_ID; }
            const std::string& GetName() const { return m_Name; }
            void SetName(const std::string& name) { m_Name = name; }

            Scene* GetScene() const { return m_Scene; }

            // Transform
            Transform& GetTransform() { return m_Transform; }
            const Transform& GetTransform() const { return m_Transform; }
            void SetTransform(const Transform& transform) { m_Transform = transform; }

            // Component management
            template<typename T>
            T* AddComponent() {
                static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
                auto component = std::make_unique<T>();
                T* ptr = component.get();
                component->SetEntity(this);
                m_Components.push_back(std::move(component));
                ptr->OnAttach();
                return ptr;
            }

            template<typename T>
            T* GetComponent() {
                static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
                for (auto& comp : m_Components) {
                    if (auto* casted = dynamic_cast<T*>(comp.get())) {
                        return casted;
                    }
                }
                return nullptr;
            }

            template<typename T>
            std::vector<T*> GetComponents() {
                static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
                std::vector<T*> result;
                for (auto& comp : m_Components) {
                    if (auto* casted = dynamic_cast<T*>(comp.get())) {
                        result.push_back(casted);
                    }
                }
                return result;
            }

            void RemoveComponent(Component* component);
            const std::vector<std::unique_ptr<Component>>& GetComponents() const { return m_Components; }

            // Hierarchy
            void SetParent(Entity* parent);
            Entity* GetParent() const { return m_Parent; }
            const std::vector<Entity*>& GetChildren() const { return m_Children; }

            // Bounds
            AABB GetBounds() const;
            AABB GetWorldBounds() const;

            // Active state
            void SetActive(bool active) { m_Active = active; }
            bool IsActive() const { return m_Active; }

        private:
            Scene* m_Scene;
            uint64_t m_ID;
            std::string m_Name;
            Transform m_Transform;
            std::vector<std::unique_ptr<Component>> m_Components;
            Entity* m_Parent = nullptr;
            std::vector<Entity*> m_Children;
            bool m_Active = true;
        };

        // Octree node for spatial indexing
        class FE_RESOURCES_API OctreeNode {
        public:
            OctreeNode(const AABB& bounds, uint32_t depth = 0, uint32_t maxDepth = 8);
            ~OctreeNode();

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            OctreeNode(const OctreeNode&) = delete;
            OctreeNode& operator=(const OctreeNode&) = delete;

            // Allow move constructor and move assignment operator
            OctreeNode(OctreeNode&&) noexcept = default;
            OctreeNode& operator=(OctreeNode&&) noexcept = default;

            void Insert(Entity* entity);
            void Remove(Entity* entity);
            void Query(const AABB& bounds, std::vector<Entity*>& results) const;
            void QueryFrustum(const glm::mat4& viewProj, std::vector<Entity*>& results) const;
            void Clear();
            void Update();

            const AABB& GetBounds() const { return m_Bounds; }
            uint32_t GetEntityCount() const { return static_cast<uint32_t>(m_Entities.size()); }

        private:
            void Subdivide();
            bool IsLeaf() const { return m_Children[0] == nullptr; }
            int GetChildIndex(const glm::vec3& point) const;
            bool IntersectsFrustum(const glm::mat4& viewProj) const;

            AABB m_Bounds;
            uint32_t m_Depth;
            uint32_t m_MaxDepth;
            std::vector<Entity*> m_Entities;
            std::array<std::unique_ptr<OctreeNode>, 8> m_Children;
            static constexpr uint32_t MAX_ENTITIES_PER_NODE = 10;
        };

        // Scene class
        class FE_RESOURCES_API Scene {
        public:
            Scene(const std::string& name = "Scene");
            ~Scene();

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            Scene(const Scene&) = delete;
            Scene& operator=(const Scene&) = delete;

            // Allow move constructor and move assignment operator
            Scene(Scene&&) noexcept = default;
            Scene& operator=(Scene&&) noexcept = default;

            const std::string& GetName() const { return m_Name; }
            void SetName(const std::string& name) { m_Name = name; }

            // Scene level management
            SceneLevel* CreateLevel(const std::string& name, uint32_t order = 0);
            SceneLevel* GetLevel(const std::string& name) const;
            SceneLevel* GetLevel(uint32_t index) const;
            void RemoveLevel(SceneLevel* level);
            void RemoveLevel(const std::string& name);
            const std::vector<std::unique_ptr<SceneLevel>>& GetLevels() const { return m_Levels; }
            std::vector<SceneLevel*> GetLevelsSortedByOrder() const;

            // Entity management (delegated to levels, but kept for compatibility)
            Entity* CreateEntity(const std::string& name = "", const std::string& levelName = "Default");
            Entity* GetEntity(uint64_t id) const;
            Entity* FindEntityByName(const std::string& name) const;
            void DestroyEntity(Entity* entity);
            void DestroyEntity(uint64_t id);
            
            // Get all entities from all levels
            std::vector<Entity*> GetAllEntities() const;

            // Spatial queries
            std::vector<Entity*> QueryBounds(const AABB& bounds) const;
            std::vector<Entity*> QueryFrustum(const glm::mat4& viewProjMatrix) const;
            std::vector<Entity*> QueryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 1000.0f) const;

            // Component queries
            std::vector<ModelComponent*> GetModelComponents() const;
            std::vector<LightComponent*> GetLightComponents() const;
            std::vector<EffectComponent*> GetEffectComponents() const;

            // Octree management
            void RebuildOctree();
            void SetOctreeBounds(const AABB& bounds);
            const AABB& GetOctreeBounds() const { return m_OctreeBounds; }

            // Scene bounds
            AABB GetSceneBounds() const;

            // Update (for dynamic objects)
            void Update(float deltaTime);

        private:
            std::string m_Name;
            std::vector<std::unique_ptr<SceneLevel>> m_Levels;
            std::unordered_map<std::string, SceneLevel*> m_LevelMap;
            
            // Entity storage (Scene owns entities, levels reference them)
            std::vector<std::unique_ptr<Entity>> m_Entities;
            std::unordered_map<uint64_t, Entity*> m_EntityMap; // Global entity map for quick lookup
            std::unordered_map<std::string, Entity*> m_EntityNameMap;
            uint64_t m_NextEntityID = 1;

            // Spatial indexing
            std::unique_ptr<OctreeNode> m_Octree;
            AABB m_OctreeBounds;
            bool m_OctreeNeedsRebuild = true;
        };

        // Scene loader/saver
        class FE_RESOURCES_API SceneLoader {
        public:
            static bool SaveToFile(const std::string& filepath, const Scene& scene);
            static bool LoadFromFile(const std::string& filepath, Scene& scene);
            static bool SaveToJSON(const std::string& filepath, const Scene& scene);
            static bool LoadFromJSON(const std::string& filepath, Scene& scene);
        };

    } // namespace Resources
} // namespace FirstEngine
