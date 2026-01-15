#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/SceneLevel.h"
#include "FirstEngine/Resources/LightComponent.h"
#include "FirstEngine/Resources/EffectComponent.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace FirstEngine {
    namespace Resources {

        // Transform implementation
        glm::mat4 Transform::GetMatrix() const {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 rotationMat = glm::mat4_cast(rotation);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
            return translation * rotationMat * scaleMat;
        }

        glm::vec3 Transform::GetForward() const {
            return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
        }

        glm::vec3 Transform::GetRight() const {
            return glm::normalize(rotation * glm::vec3(1.0f, 0.0f, 0.0f));
        }

        glm::vec3 Transform::GetUp() const {
            return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f));
        }

        // AABB implementation
        bool AABB::Contains(const glm::vec3& point) const {
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y &&
                   point.z >= min.z && point.z <= max.z;
        }

        bool AABB::Intersects(const AABB& other) const {
            return min.x <= other.max.x && max.x >= other.min.x &&
                   min.y <= other.max.y && max.y >= other.min.y &&
                   min.z <= other.max.z && max.z >= other.min.z;
        }

        AABB AABB::Transform(const glm::mat4& transform) const {
            glm::vec3 corners[8] = {
                glm::vec3(min.x, min.y, min.z),
                glm::vec3(max.x, min.y, min.z),
                glm::vec3(min.x, max.y, min.z),
                glm::vec3(max.x, max.y, min.z),
                glm::vec3(min.x, min.y, max.z),
                glm::vec3(max.x, min.y, max.z),
                glm::vec3(min.x, max.y, max.z),
                glm::vec3(max.x, max.y, max.z)
            };

            glm::vec3 transformedMin = glm::vec3(transform * glm::vec4(corners[0], 1.0f));
            glm::vec3 transformedMax = transformedMin;

            for (int i = 1; i < 8; ++i) {
                glm::vec3 transformed = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
                transformedMin = glm::min(transformedMin, transformed);
                transformedMax = glm::max(transformedMax, transformed);
            }

            return AABB(transformedMin, transformedMax);
        }


        // LightComponent implementation
        LightComponent::LightComponent() : Component(ComponentType::Light) {}

        // EffectComponent implementation
        EffectComponent::EffectComponent() : Component(ComponentType::Effect) {}

        // Entity implementation
        Entity::Entity(Scene* scene, uint64_t id, const std::string& name)
            : m_Scene(scene), m_ID(id), m_Name(name) {}

        Entity::~Entity() {
            for (auto& comp : m_Components) {
                comp->OnDetach();
            }
            if (m_Parent) {
                auto& siblings = m_Parent->m_Children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
            }
            for (auto* child : m_Children) {
                child->m_Parent = nullptr;
            }
        }

        void Entity::RemoveComponent(Component* component) {
            auto it = std::find_if(m_Components.begin(), m_Components.end(),
                [component](const std::unique_ptr<Component>& comp) {
                    return comp.get() == component;
                });
            if (it != m_Components.end()) {
                (*it)->OnDetach();
                m_Components.erase(it);
            }
        }

        void Entity::SetParent(Entity* parent) {
            if (m_Parent == parent) return;

            if (m_Parent) {
                auto& siblings = m_Parent->m_Children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
            }

            m_Parent = parent;
            if (m_Parent) {
                m_Parent->m_Children.push_back(this);
            }
        }

        AABB Entity::GetBounds() const {
            AABB bounds;
            bool hasBounds = false;

            for (const auto& comp : m_Components) {
                AABB compBounds = comp->GetBounds();
                if (compBounds.min != compBounds.max || 
                    (compBounds.min != glm::vec3(0.0f) || compBounds.max != glm::vec3(0.0f))) {
                    if (!hasBounds) {
                        bounds = compBounds;
                        hasBounds = true;
                    } else {
                        bounds.min = glm::min(bounds.min, compBounds.min);
                        bounds.max = glm::max(bounds.max, compBounds.max);
                    }
                }
            }

            if (!hasBounds) {
                // Default bounds around position
                bounds = AABB(m_Transform.position - glm::vec3(0.5f), m_Transform.position + glm::vec3(0.5f));
            }

            return bounds;
        }

        AABB Entity::GetWorldBounds() const {
            AABB localBounds = GetBounds();
            glm::mat4 worldMatrix = m_Transform.GetMatrix();
            
            // Apply parent transforms
            Entity* parent = m_Parent;
            while (parent) {
                worldMatrix = parent->m_Transform.GetMatrix() * worldMatrix;
                parent = parent->m_Parent;
            }

            return localBounds.Transform(worldMatrix);
        }

        // OctreeNode implementation
        OctreeNode::OctreeNode(const AABB& bounds, uint32_t depth, uint32_t maxDepth)
            : m_Bounds(bounds), m_Depth(depth), m_MaxDepth(maxDepth) {
            // Initialize all children to nullptr (cannot use fill() with unique_ptr)
            for (auto& child : m_Children) {
                child = nullptr;
            }
        }

        OctreeNode::~OctreeNode() = default;

        void OctreeNode::Insert(Entity* entity) {
            if (!entity) return;

            AABB entityBounds = entity->GetWorldBounds();
            if (!m_Bounds.Intersects(entityBounds)) {
                return; // Entity outside this node
            }

            if (IsLeaf()) {
                m_Entities.push_back(entity);
                if (m_Entities.size() > MAX_ENTITIES_PER_NODE && m_Depth < m_MaxDepth) {
                    Subdivide();
                }
            } else {
                // Insert into appropriate child
                glm::vec3 center = entityBounds.GetCenter();
                int childIndex = GetChildIndex(center);
                if (m_Children[childIndex]) {
                    m_Children[childIndex]->Insert(entity);
                }
            }
        }

        void OctreeNode::Remove(Entity* entity) {
            if (!entity) return;

            auto it = std::find(m_Entities.begin(), m_Entities.end(), entity);
            if (it != m_Entities.end()) {
                m_Entities.erase(it);
                return;
            }

            // Search in children
            if (!IsLeaf()) {
                glm::vec3 center = entity->GetWorldBounds().GetCenter();
                int childIndex = GetChildIndex(center);
                if (m_Children[childIndex]) {
                    m_Children[childIndex]->Remove(entity);
                }
            }
        }

        void OctreeNode::Query(const AABB& bounds, std::vector<Entity*>& results) const {
            if (!m_Bounds.Intersects(bounds)) {
                return;
            }

            for (Entity* entity : m_Entities) {
                if (bounds.Intersects(entity->GetWorldBounds())) {
                    results.push_back(entity);
                }
            }

            if (!IsLeaf()) {
                for (const auto& child : m_Children) {
                    if (child) {
                        child->Query(bounds, results);
                    }
                }
            }
        }

        void OctreeNode::QueryFrustum(const glm::mat4& viewProj, std::vector<Entity*>& results) const {
            if (!IntersectsFrustum(viewProj)) {
                return;
            }

            for (Entity* entity : m_Entities) {
                AABB bounds = entity->GetWorldBounds();
                if (IntersectsFrustum(viewProj)) {
                    results.push_back(entity);
                }
            }

            if (!IsLeaf()) {
                for (const auto& child : m_Children) {
                    if (child) {
                        child->QueryFrustum(viewProj, results);
                    }
                }
            }
        }

        void OctreeNode::Clear() {
            m_Entities.clear();
            for (auto& child : m_Children) {
                if (child) {
                    child->Clear();
                    child.reset();
                }
            }
        }

        void OctreeNode::Update() {
            // Remove entities that are no longer in bounds
            m_Entities.erase(
                std::remove_if(m_Entities.begin(), m_Entities.end(),
                    [this](Entity* entity) {
                        return !m_Bounds.Intersects(entity->GetWorldBounds());
                    }),
                m_Entities.end()
            );

            if (!IsLeaf()) {
                for (auto& child : m_Children) {
                    if (child) {
                        child->Update();
                    }
                }
            }
        }

        void OctreeNode::Subdivide() {
            glm::vec3 center = m_Bounds.GetCenter();
            glm::vec3 halfSize = m_Bounds.GetHalfSize();
            glm::vec3 quarterSize = halfSize * 0.5f;

            // Create 8 children
            for (int i = 0; i < 8; ++i) {
                glm::vec3 offset(
                    (i & 1) ? quarterSize.x : -quarterSize.x,
                    (i & 2) ? quarterSize.y : -quarterSize.y,
                    (i & 4) ? quarterSize.z : -quarterSize.z
                );
                AABB childBounds(center + offset - quarterSize, center + offset + quarterSize);
                m_Children[i] = std::make_unique<OctreeNode>(childBounds, m_Depth + 1, m_MaxDepth);
            }

            // Redistribute entities
            std::vector<Entity*> entitiesToRedistribute = std::move(m_Entities);
            for (Entity* entity : entitiesToRedistribute) {
                glm::vec3 entityCenter = entity->GetWorldBounds().GetCenter();
                int childIndex = GetChildIndex(entityCenter);
                m_Children[childIndex]->Insert(entity);
            }
        }

        int OctreeNode::GetChildIndex(const glm::vec3& point) const {
            glm::vec3 center = m_Bounds.GetCenter();
            int index = 0;
            if (point.x >= center.x) index |= 1;
            if (point.y >= center.y) index |= 2;
            if (point.z >= center.z) index |= 4;
            return index;
        }

        bool OctreeNode::IntersectsFrustum(const glm::mat4& viewProj) const {
            // Simplified frustum test using AABB corners
            glm::vec3 corners[8] = {
                glm::vec3(m_Bounds.min.x, m_Bounds.min.y, m_Bounds.min.z),
                glm::vec3(m_Bounds.max.x, m_Bounds.min.y, m_Bounds.min.z),
                glm::vec3(m_Bounds.min.x, m_Bounds.max.y, m_Bounds.min.z),
                glm::vec3(m_Bounds.max.x, m_Bounds.max.y, m_Bounds.min.z),
                glm::vec3(m_Bounds.min.x, m_Bounds.min.y, m_Bounds.max.z),
                glm::vec3(m_Bounds.max.x, m_Bounds.min.y, m_Bounds.max.z),
                glm::vec3(m_Bounds.min.x, m_Bounds.max.y, m_Bounds.max.z),
                glm::vec3(m_Bounds.max.x, m_Bounds.max.y, m_Bounds.max.z)
            };

            bool allInside = true;
            bool allOutside = true;

            for (const auto& corner : corners) {
                glm::vec4 clipPos = viewProj * glm::vec4(corner, 1.0f);
                bool inside = clipPos.x >= -clipPos.w && clipPos.x <= clipPos.w &&
                             clipPos.y >= -clipPos.w && clipPos.y <= clipPos.w &&
                             clipPos.z >= -clipPos.w && clipPos.z <= clipPos.w &&
                             clipPos.w > 0.0f;

                if (inside) allOutside = false;
                else allInside = false;
            }

            return !allOutside; // Intersects if not all outside
        }

        // Scene implementation
        Scene::Scene(const std::string& name) : m_Name(name) {
            // Default octree bounds (will be expanded as needed)
            m_OctreeBounds = AABB(glm::vec3(-100.0f), glm::vec3(100.0f));
            m_Octree = std::make_unique<OctreeNode>(m_OctreeBounds);
            
            // Create default level
            CreateLevel("Default", 0);
        }

        Scene::~Scene() = default;

        SceneLevel* Scene::CreateLevel(const std::string& name, uint32_t order) {
            if (m_LevelMap.find(name) != m_LevelMap.end()) {
                return m_LevelMap[name]; // Level already exists
            }
            
            auto level = std::make_unique<SceneLevel>(this, name, order);
            SceneLevel* ptr = level.get();
            m_Levels.push_back(std::move(level));
            m_LevelMap[name] = ptr;
            return ptr;
        }

        SceneLevel* Scene::GetLevel(const std::string& name) const {
            auto it = m_LevelMap.find(name);
            return it != m_LevelMap.end() ? it->second : nullptr;
        }

        SceneLevel* Scene::GetLevel(uint32_t index) const {
            if (index >= m_Levels.size()) return nullptr;
            return m_Levels[index].get();
        }

        void Scene::RemoveLevel(SceneLevel* level) {
            if (!level) return;
            RemoveLevel(level->GetName());
        }

        void Scene::RemoveLevel(const std::string& name) {
            auto it = std::find_if(m_Levels.begin(), m_Levels.end(),
                [&name](const std::unique_ptr<SceneLevel>& l) { return l->GetName() == name; });
            if (it != m_Levels.end()) {
                m_LevelMap.erase(name);
                m_Levels.erase(it);
            }
        }

        std::vector<SceneLevel*> Scene::GetLevelsSortedByOrder() const {
            std::vector<SceneLevel*> sorted;
            sorted.reserve(m_Levels.size());
            for (const auto& level : m_Levels) {
                sorted.push_back(level.get());
            }
            std::sort(sorted.begin(), sorted.end(),
                [](SceneLevel* a, SceneLevel* b) {
                    return a->GetOrder() < b->GetOrder();
                });
            return sorted;
        }

        Entity* Scene::CreateEntity(const std::string& name, const std::string& levelName) {
            SceneLevel* level = GetLevel(levelName.empty() ? "Default" : levelName);
            if (!level) {
                level = CreateLevel(levelName.empty() ? "Default" : levelName, 0);
            }
            if (!level) return nullptr;
            
            uint64_t id = m_NextEntityID++;
            std::string entityName = name.empty() ? "Entity_" + std::to_string(id) : name;
            auto entity = std::make_unique<Entity>(this, id, entityName);
            Entity* ptr = entity.get();
            
            // Store in scene's storage (Scene owns entities)
            m_Entities.push_back(std::move(entity));
            m_EntityMap[id] = ptr;
            if (!entityName.empty()) {
                m_EntityNameMap[entityName] = ptr;
            }
            
            // Add to level (level just references)
            level->AddEntity(ptr);
            
            m_OctreeNeedsRebuild = true;
            return ptr;
        }

        Entity* Scene::GetEntity(uint64_t id) const {
            auto it = m_EntityMap.find(id);
            return it != m_EntityMap.end() ? it->second : nullptr;
        }

        Entity* Scene::FindEntityByName(const std::string& name) const {
            auto it = m_EntityNameMap.find(name);
            return it != m_EntityNameMap.end() ? it->second : nullptr;
        }

        void Scene::DestroyEntity(Entity* entity) {
            if (!entity) return;
            DestroyEntity(entity->GetID());
        }

        void Scene::DestroyEntity(uint64_t id) {
            auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
                [id](const std::unique_ptr<Entity>& e) { return e->GetID() == id; });
            if (it == m_Entities.end()) return;
            
            Entity* entity = it->get();
            
            // Remove from all levels
            for (auto& level : m_Levels) {
                level->RemoveEntity(id);
            }
            
            // Remove from maps
            m_EntityMap.erase(id);
            if (!entity->GetName().empty()) {
                m_EntityNameMap.erase(entity->GetName());
            }
            
            // Remove from storage (entity will be destroyed when unique_ptr is destroyed)
            m_Entities.erase(it);
            
            m_OctreeNeedsRebuild = true;
        }
        
        std::vector<Entity*> Scene::GetAllEntities() const {
            std::vector<Entity*> entities;
            for (const auto& level : m_Levels) {
                const auto& levelEntities = level->GetEntities();
                entities.insert(entities.end(), levelEntities.begin(), levelEntities.end());
            }
            // Remove duplicates
            std::sort(entities.begin(), entities.end());
            entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
            return entities;
        }

        std::vector<Entity*> Scene::QueryBounds(const AABB& bounds) const {
            if (m_OctreeNeedsRebuild) {
                const_cast<Scene*>(this)->RebuildOctree();
            }
            std::vector<Entity*> results;
            if (m_Octree) {
                m_Octree->Query(bounds, results);
            }
            return results;
        }

        std::vector<Entity*> Scene::QueryFrustum(const glm::mat4& viewProjMatrix) const {
            if (m_OctreeNeedsRebuild) {
                const_cast<Scene*>(this)->RebuildOctree();
            }
            std::vector<Entity*> results;
            if (m_Octree) {
                m_Octree->QueryFrustum(viewProjMatrix, results);
            }
            return results;
        }

        std::vector<Entity*> Scene::QueryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
            // Simple ray-AABB intersection test
            std::vector<Entity*> candidates = QueryBounds(GetSceneBounds());
            std::vector<Entity*> results;

            glm::vec3 normalizedDir = glm::normalize(direction);
            for (Entity* entity : candidates) {
                AABB bounds = entity->GetWorldBounds();
                glm::vec3 min = bounds.min;
                glm::vec3 max = bounds.max;

                float tmin = 0.0f;
                float tmax = maxDistance;

                for (int i = 0; i < 3; ++i) {
                    float invD = 1.0f / normalizedDir[i];
                    float t0 = (min[i] - origin[i]) * invD;
                    float t1 = (max[i] - origin[i]) * invD;
                    if (invD < 0.0f) std::swap(t0, t1);
                    tmin = t0 > tmin ? t0 : tmin;
                    tmax = t1 < tmax ? t1 : tmax;
                    if (tmax < tmin) break;
                }

                if (tmin < tmax && tmin >= 0.0f) {
                    results.push_back(entity);
                }
            }

            return results;
        }

        std::vector<ModelComponent*> Scene::GetModelComponents() const {
            std::vector<ModelComponent*> components;
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                if (auto* model = entity->GetComponent<ModelComponent>()) {
                    components.push_back(model);
                }
            }
            return components;
        }

        std::vector<LightComponent*> Scene::GetLightComponents() const {
            std::vector<LightComponent*> components;
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                if (auto* light = entity->GetComponent<LightComponent>()) {
                    components.push_back(light);
                }
            }
            return components;
        }

        std::vector<EffectComponent*> Scene::GetEffectComponents() const {
            std::vector<EffectComponent*> components;
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                if (auto* effect = entity->GetComponent<EffectComponent>()) {
                    components.push_back(effect);
                }
            }
            return components;
        }

        void Scene::RebuildOctree() {
            if (!m_Octree) {
                m_Octree = std::make_unique<OctreeNode>(m_OctreeBounds);
            } else {
                m_Octree->Clear();
            }

            // Expand octree bounds to include all entities
            AABB sceneBounds = GetSceneBounds();
            if (sceneBounds.min != sceneBounds.max) {
                glm::vec3 center = sceneBounds.GetCenter();
                glm::vec3 size = sceneBounds.GetSize();
                glm::vec3 halfSize = size * 0.5f;
                // Add padding
                halfSize *= 1.2f;
                m_OctreeBounds = AABB(center - halfSize, center + halfSize);
                m_Octree = std::make_unique<OctreeNode>(m_OctreeBounds);
            }

            // Insert all entities from all levels
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                if (entity && entity->IsActive()) {
                    m_Octree->Insert(entity);
                }
            }

            m_OctreeNeedsRebuild = false;
        }

        void Scene::SetOctreeBounds(const AABB& bounds) {
            m_OctreeBounds = bounds;
            m_OctreeNeedsRebuild = true;
        }

        AABB Scene::GetSceneBounds() const {
            std::vector<Entity*> entities = GetAllEntities();
            if (entities.empty()) {
                return AABB(glm::vec3(-10.0f), glm::vec3(10.0f));
            }

            AABB bounds = entities[0]->GetWorldBounds();
            for (size_t i = 1; i < entities.size(); ++i) {
                AABB entityBounds = entities[i]->GetWorldBounds();
                bounds.min = glm::min(bounds.min, entityBounds.min);
                bounds.max = glm::max(bounds.max, entityBounds.max);
            }

            return bounds;
        }

        void Scene::Update(float deltaTime) {
            // Update dynamic objects in octree
            if (m_Octree && !m_OctreeNeedsRebuild) {
                m_Octree->Update();
            }
        }

        // SceneLoader implementation (stub - JSON implementation would go here)
        bool SceneLoader::SaveToFile(const std::string& filepath, const Scene& scene) {
            return SaveToJSON(filepath, scene);
        }

        bool SceneLoader::LoadFromFile(const std::string& filepath, Scene& scene) {
            return LoadFromJSON(filepath, scene);
        }

        bool SceneLoader::SaveToJSON(const std::string& filepath, const Scene& scene) {
            // TODO: Implement JSON serialization using a library like nlohmann/json or rapidjson
            // For now, return false to indicate not implemented
            return false;
        }

        bool SceneLoader::LoadFromJSON(const std::string& filepath, Scene& scene) {
            // TODO: Implement JSON deserialization
            return false;
        }

    } // namespace Resources
} // namespace FirstEngine
