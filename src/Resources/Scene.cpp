#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/SceneLevel.h"
#include "FirstEngine/Resources/LightComponent.h"
#include "FirstEngine/Resources/EffectComponent.h"
#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Resources/CameraComponent.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

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
            return point.x >= minBounds.x && point.x <= maxBounds.x &&
                   point.y >= minBounds.y && point.y <= maxBounds.y &&
                   point.z >= minBounds.z && point.z <= maxBounds.z;
        }

        bool AABB::Intersects(const AABB& other) const {
            return minBounds.x <= other.maxBounds.x && maxBounds.x >= other.minBounds.x &&
                   minBounds.y <= other.maxBounds.y && maxBounds.y >= other.minBounds.y &&
                   minBounds.z <= other.maxBounds.z && maxBounds.z >= other.minBounds.z;
        }

        AABB AABB::Transform(const glm::mat4& transform) const {
            glm::vec3 corners[8] = {
                glm::vec3(minBounds.x, minBounds.y, minBounds.z),
                glm::vec3(maxBounds.x, minBounds.y, minBounds.z),
                glm::vec3(minBounds.x, maxBounds.y, minBounds.z),
                glm::vec3(maxBounds.x, maxBounds.y, minBounds.z),
                glm::vec3(minBounds.x, minBounds.y, maxBounds.z),
                glm::vec3(maxBounds.x, minBounds.y, maxBounds.z),
                glm::vec3(minBounds.x, maxBounds.y, maxBounds.z),
                glm::vec3(maxBounds.x, maxBounds.y, maxBounds.z)
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

        void Entity::OnLoad() {
            // Call OnLoad() on all components
            // This is called when Entity is fully loaded (all components attached, resources ready)
            for (auto& comp : m_Components) {
                if (comp) {
                    comp->OnLoad();
                }
            }
            
        }

        const glm::mat4& Entity::GetWorldMatrix() const {
            if (m_WorldMatrixDirty) {
                UpdateWorldMatrix();
            }
            return m_WorldMatrix;
        }

        void Entity::UpdateWorldMatrix() const {
            if (m_Parent) {
                // World matrix = parent's world matrix * local transform
                m_WorldMatrix = m_Parent->GetWorldMatrix() * m_Transform.GetMatrix();
            } else {
                // Root entity: world matrix = local transform
                m_WorldMatrix = m_Transform.GetMatrix();
            }
            m_WorldMatrixDirty = false;
        }

        void Entity::MarkChildrenWorldMatrixDirty() {
            for (Entity* child : m_Children) {
                if (child) {
                    child->m_WorldMatrixDirty = true;
                    child->MarkChildrenWorldMatrixDirty(); // Recursively mark all descendants
                }
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
            
            // Mark world matrix as dirty when parent changes
            m_WorldMatrixDirty = true;
            MarkChildrenWorldMatrixDirty(); // Mark all children as dirty too
        }

        AABB Entity::GetBounds() const {
            AABB bounds;
            bool hasBounds = false;

            for (const auto& comp : m_Components) {
                AABB compBounds = comp->GetBounds();
                if (compBounds.minBounds != compBounds.maxBounds || 
                    (compBounds.minBounds != glm::vec3(0.0f) || compBounds.maxBounds != glm::vec3(0.0f))) {
                    if (!hasBounds) {
                        bounds = compBounds;
                        hasBounds = true;
                    } else {
                        bounds.minBounds = glm::min(bounds.minBounds, compBounds.minBounds);
                        bounds.maxBounds = glm::max(bounds.maxBounds, compBounds.maxBounds);
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
            // Use cached world matrix (automatically updated when needed)
            const glm::mat4& worldMatrix = GetWorldMatrix();
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
                glm::vec3(m_Bounds.minBounds.x, m_Bounds.minBounds.y, m_Bounds.minBounds.z),
                glm::vec3(m_Bounds.maxBounds.x, m_Bounds.minBounds.y, m_Bounds.minBounds.z),
                glm::vec3(m_Bounds.minBounds.x, m_Bounds.maxBounds.y, m_Bounds.minBounds.z),
                glm::vec3(m_Bounds.maxBounds.x, m_Bounds.maxBounds.y, m_Bounds.minBounds.z),
                glm::vec3(m_Bounds.minBounds.x, m_Bounds.minBounds.y, m_Bounds.maxBounds.z),
                glm::vec3(m_Bounds.maxBounds.x, m_Bounds.minBounds.y, m_Bounds.maxBounds.z),
                glm::vec3(m_Bounds.minBounds.x, m_Bounds.maxBounds.y, m_Bounds.maxBounds.z),
                glm::vec3(m_Bounds.maxBounds.x, m_Bounds.maxBounds.y, m_Bounds.maxBounds.z)
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
                glm::vec3 min = bounds.minBounds;
                glm::vec3 max = bounds.maxBounds;

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

        std::vector<CameraComponent*> Scene::GetCameraComponents() const {
            std::vector<CameraComponent*> components;
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                auto cameras = entity->GetComponents<CameraComponent>();
                components.insert(components.end(), cameras.begin(), cameras.end());
            }
            return components;
        }

        CameraComponent* Scene::GetMainCamera() const {
            // First, try to find a camera marked as main
            std::vector<Entity*> entities = GetAllEntities();
            for (Entity* entity : entities) {
                auto cameras = entity->GetComponents<CameraComponent>();
                for (CameraComponent* camera : cameras) {
                    if (camera && camera->IsMainCamera()) {
                        return camera;
                    }
                }
            }
            
            // If no main camera found, return the first camera
            for (Entity* entity : entities) {
                auto cameras = entity->GetComponents<CameraComponent>();
                if (!cameras.empty() && cameras[0] != nullptr) {
                    return cameras[0];
                }
            }
            
            return nullptr;
        }

        void Scene::RebuildOctree() {
            if (!m_Octree) {
                m_Octree = std::make_unique<OctreeNode>(m_OctreeBounds);
            } else {
                m_Octree->Clear();
            }

            // Expand octree bounds to include all entities
            AABB sceneBounds = GetSceneBounds();
            if (sceneBounds.minBounds != sceneBounds.maxBounds) {
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
                bounds.minBounds = glm::min(bounds.minBounds, entityBounds.minBounds);
                bounds.maxBounds = glm::max(bounds.maxBounds, entityBounds.maxBounds);
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

        // Helper function to escape JSON string
        static std::string EscapeJSONString(const std::string& str) {
            std::ostringstream o;
            for (char c : str) {
                switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                    } else {
                        o << c;
                    }
                }
            }
            return o.str();
        }

        // Helper function to unescape JSON string
        static std::string UnescapeJSONString(const std::string& str) {
            std::string result;
            for (size_t i = 0; i < str.length(); ++i) {
                if (str[i] == '\\' && i + 1 < str.length()) {
                    switch (str[i + 1]) {
                    case '"': result += '"'; ++i; break;
                    case '\\': result += '\\'; ++i; break;
                    case 'n': result += '\n'; ++i; break;
                    case 'r': result += '\r'; ++i; break;
                    case 't': result += '\t'; ++i; break;
                    case 'b': result += '\b'; ++i; break;
                    case 'f': result += '\f'; ++i; break;
                    case 'u':
                        if (i + 5 < str.length()) {
                            i += 5; // Skip \uXXXX
                        }
                        break;
                    default: result += str[i]; break;
                    }
                } else {
                    result += str[i];
                }
            }
            return result;
        }

        bool SceneLoader::SaveToJSON(const std::string& filepath, const Scene& scene) {
            try {
                std::ofstream file(filepath, std::ios::out | std::ios::trunc);
                if (!file.is_open()) {
                    return false;
                }

                file << "{\n";
                file << "  \"version\": 1,\n";
                file << "  \"name\": \"" << EscapeJSONString(scene.GetName()) << "\",\n";
                file << "  \"levels\": [\n";

                // Save levels
                const auto& levels = scene.GetLevels();
                bool firstLevel = true;
                for (const auto& levelPtr : levels) {
                    if (!levelPtr) continue;
                    const SceneLevel* level = levelPtr.get();

                    if (!firstLevel) {
                        file << ",\n";
                    }
                    firstLevel = false;

                    file << "    {\n";
                    file << "      \"name\": \"" << EscapeJSONString(level->GetName()) << "\",\n";
                    file << "      \"order\": " << level->GetOrder() << ",\n";
                    file << "      \"visible\": " << (level->IsVisible() ? "true" : "false") << ",\n";
                    file << "      \"enabled\": " << (level->IsEnabled() ? "true" : "false") << ",\n";
                    file << "      \"entities\": [\n";

                    // Save entities
                    const auto& entities = level->GetEntities();
                    bool firstEntity = true;
                    for (Entity* entity : entities) {
                        if (!entity) continue;

                        if (!firstEntity) {
                            file << ",\n";
                        }
                        firstEntity = false;

                        file << "        {\n";
                        file << "          \"id\": " << entity->GetID() << ",\n";
                        file << "          \"name\": \"" << EscapeJSONString(entity->GetName()) << "\",\n";
                        file << "          \"active\": " << (entity->IsActive() ? "true" : "false") << ",\n";

                        // Save transform
                        const Transform& transform = entity->GetTransform();
                        file << "          \"transform\": {\n";
                        file << "            \"position\": [" << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << "],\n";
                        file << "            \"rotation\": [" << transform.rotation.w << ", " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << "],\n";
                        file << "            \"scale\": [" << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << "]\n";
                        file << "          },\n";

                        // Save components
                        file << "          \"components\": [\n";
                        const auto& components = entity->GetComponents();
                        bool firstComponent = true;
                        for (const auto& comp : components) {
                            if (!comp) continue;

                            if (!firstComponent) {
                                file << ",\n";
                            }
                            firstComponent = false;

                            file << "            {\n";
                            file << "              \"type\": \"" << static_cast<int>(comp->GetType()) << "\",\n";

                            // Save ModelComponent
                            // Note: ModelComponent uses ComponentType::Mesh
                            if (comp->GetType() == ComponentType::Mesh) {
                                if (auto* modelComp = dynamic_cast<const ModelComponent*>(comp.get())) {
                                    ModelHandle model = modelComp->GetModel();
                                    if (model) {
                                        ResourceID modelID = model->GetMetadata().resourceID;
                                        file << "              \"modelID\": " << modelID << ",\n";
                                    }
                                }
                            }
                            
                            // Save CameraComponent
                            // Editor uses type 3 for Camera
                            if (comp->GetType() == ComponentType::Camera) {
                                if (auto* cameraComp = dynamic_cast<const CameraComponent*>(comp.get())) {
                                    file << "              \"fov\": " << cameraComp->GetFOV() << ",\n";
                                    file << "              \"near\": " << cameraComp->GetNear() << ",\n";
                                    file << "              \"far\": " << cameraComp->GetFar() << ",\n";
                                    file << "              \"isMainCamera\": " << (cameraComp->IsMainCamera() ? "true" : "false") << ",\n";
                                }
                            }

                            file << "            }";
                        }
                        file << "\n          ]\n";
                        file << "        }";
                    }

                    file << "\n      ]\n";
                    file << "    }";
                }

                file << "\n  ]\n";
                file << "}\n";

                file.close();
                return true;
            } catch (...) {
                return false;
            }
        }

        // Helper function to find matching closing brace for nested objects
        static size_t FindMatchingBrace(const std::string& str, size_t startPos) {
            if (startPos >= str.length() || str[startPos] != '{') {
                return std::string::npos;
            }
            
            int depth = 1;
            size_t pos = startPos + 1;
            bool inString = false;
            bool escaped = false;
            
            while (pos < str.length() && depth > 0) {
                char c = str[pos];
                
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"' && !escaped) {
                    inString = !inString;
                } else if (!inString) {
                    if (c == '{') {
                        depth++;
                    } else if (c == '}') {
                        depth--;
                        if (depth == 0) {
                            return pos;
                        }
                    }
                }
                pos++;
            }
            
            return std::string::npos;
        }

        bool SceneLoader::LoadFromJSON(const std::string& filepath, Scene& scene) {
            try {
                std::ifstream file(filepath, std::ios::in);
                if (!file.is_open()) {
                    return false;
                }

                // Read entire file
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                file.close();

                // Simple JSON parser
                // Find scene name
                size_t namePos = content.find("\"name\"");
                if (namePos != std::string::npos) {
                    size_t colonPos = content.find(':', namePos);
                    size_t quoteStart = content.find('"', colonPos);
                    size_t quoteEnd = content.find('"', quoteStart + 1);
                    if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                        std::string nameStr = content.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                        scene.SetName(UnescapeJSONString(nameStr));
                    }
                }

                // Find levels array
                size_t levelsPos = content.find("\"levels\"");
                if (levelsPos == std::string::npos) {
                    return false;
                }

                size_t arrayStart = content.find('[', levelsPos);
                if (arrayStart == std::string::npos) {
                    return false;
                }

                // Parse levels
                size_t pos = arrayStart + 1;
                ResourceManager& resourceManager = ResourceManager::GetInstance();

                while (pos < content.length()) {
                    size_t levelStart = content.find('{', pos);
                    if (levelStart == std::string::npos) break;

                    // Use matching brace function to find the correct end of nested object
                    size_t levelEnd = FindMatchingBrace(content, levelStart);
                    if (levelEnd == std::string::npos) break;

                    std::string levelStr = content.substr(levelStart, levelEnd - levelStart + 1);

                    // Parse level name
                    size_t namePos = levelStr.find("\"name\"");
                    size_t orderPos = levelStr.find("\"order\"");
                    size_t entitiesPos = levelStr.find("\"entities\"");

                    if (namePos != std::string::npos) {
                        size_t colonPos = levelStr.find(':', namePos);
                        size_t quoteStart = levelStr.find('"', colonPos);
                        size_t quoteEnd = levelStr.find('"', quoteStart + 1);
                        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                            std::string levelName = UnescapeJSONString(levelStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1));

                            // Parse order
                            uint32_t order = 0;
                            if (orderPos != std::string::npos) {
                                size_t colonPos = levelStr.find(':', orderPos);
                                size_t valueStart = levelStr.find_first_not_of(" \t\n\r", colonPos + 1);
                                size_t valueEnd = levelStr.find_first_of(",}\n\r", valueStart);
                                if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                    try {
                                        order = std::stoul(levelStr.substr(valueStart, valueEnd - valueStart));
                                    } catch (...) {}
                                }
                            }

                            SceneLevel* level = scene.CreateLevel(levelName, order);

                            // Parse entities
                            if (entitiesPos != std::string::npos) {
                                size_t entitiesArrayStart = levelStr.find('[', entitiesPos);
                                if (entitiesArrayStart != std::string::npos) {
                                    size_t entityPos = entitiesArrayStart + 1;
                                    while (entityPos < levelStr.length()) {
                                        size_t entityStart = levelStr.find('{', entityPos);
                                        if (entityStart == std::string::npos) break;

                                        // Use matching brace function to find the correct end of nested object
                                        size_t entityEnd = FindMatchingBrace(levelStr, entityStart);
                                        if (entityEnd == std::string::npos) break;

                                        std::string entityStr = levelStr.substr(entityStart, entityEnd - entityStart + 1);

                                        // Parse entity
                                        size_t idPos = entityStr.find("\"id\"");
                                        size_t namePos = entityStr.find("\"name\"");
                                        size_t transformPos = entityStr.find("\"transform\"");
                                        size_t componentsPos = entityStr.find("\"components\"");

                                        if (idPos != std::string::npos && namePos != std::string::npos) {
                                            // Parse ID
                                            size_t colonPos = entityStr.find(':', idPos);
                                            size_t valueStart = entityStr.find_first_not_of(" \t\n\r", colonPos + 1);
                                            size_t valueEnd = entityStr.find_first_of(",}\n\r", valueStart);
                                            uint64_t entityID = 0;
                                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                                try {
                                                    entityID = std::stoull(entityStr.substr(valueStart, valueEnd - valueStart));
                                                } catch (...) {}
                                            }

                                            // Parse name
                                            colonPos = entityStr.find(':', namePos);
                                            quoteStart = entityStr.find('"', colonPos);
                                            size_t quoteEnd = entityStr.find('"', quoteStart + 1);
                                            std::string entityName;
                                            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                                                entityName = UnescapeJSONString(entityStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1));
                                            }

                                            Entity* entity = level->CreateEntity(entityName);
                                            if (!entity) continue;

                                            // Parse transform
                                            if (transformPos != std::string::npos) {
                                                size_t transformObjStart = entityStr.find('{', transformPos);
                                                size_t transformObjEnd = FindMatchingBrace(entityStr, transformObjStart);
                                                if (transformObjStart != std::string::npos && transformObjEnd != std::string::npos) {
                                                    std::string transformStr = entityStr.substr(transformObjStart, transformObjEnd - transformObjStart + 1);

                                                    // Parse position
                                                    size_t posPos = transformStr.find("\"position\"");
                                                    if (posPos != std::string::npos) {
                                                        size_t arrayStart = transformStr.find('[', posPos);
                                                        size_t arrayEnd = transformStr.find(']', arrayStart);
                                                        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                                                            std::string arrayStr = transformStr.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                                                            std::istringstream iss(arrayStr);
                                                            char comma;
                                                            iss >> entity->GetTransform().position.x >> comma >> entity->GetTransform().position.y >> comma >> entity->GetTransform().position.z;
                                                        }
                                                    }

                                                    // Parse rotation
                                                    size_t rotPos = transformStr.find("\"rotation\"");
                                                    if (rotPos != std::string::npos) {
                                                        size_t arrayStart = transformStr.find('[', rotPos);
                                                        size_t arrayEnd = transformStr.find(']', arrayStart);
                                                        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                                                            std::string arrayStr = transformStr.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                                                            std::istringstream iss(arrayStr);
                                                            char comma;
                                                            iss >> entity->GetTransform().rotation.w >> comma >> entity->GetTransform().rotation.x >> comma >> entity->GetTransform().rotation.y >> comma >> entity->GetTransform().rotation.z;
                                                        }
                                                    }

                                                    // Parse scale
                                                    size_t scalePos = transformStr.find("\"scale\"");
                                                    if (scalePos != std::string::npos) {
                                                        size_t arrayStart = transformStr.find('[', scalePos);
                                                        size_t arrayEnd = transformStr.find(']', arrayStart);
                                                        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                                                            std::string arrayStr = transformStr.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                                                            std::istringstream iss(arrayStr);
                                                            char comma;
                                                            iss >> entity->GetTransform().scale.x >> comma >> entity->GetTransform().scale.y >> comma >> entity->GetTransform().scale.z;
                                                        }
                                                    }
                                                }
                                            }

                                            // Parse components
                                            if (componentsPos != std::string::npos) {
                                                size_t componentsArrayStart = entityStr.find('[', componentsPos);
                                                if (componentsArrayStart != std::string::npos) {
                                                    size_t compPos = componentsArrayStart + 1;
                                                    while (compPos < entityStr.length()) {
                                                        size_t compStart = entityStr.find('{', compPos);
                                                        if (compStart == std::string::npos) break;

                                                        size_t compEnd = entityStr.find('}', compStart);
                                                        if (compEnd == std::string::npos) break;

                                                        std::string compStr = entityStr.substr(compStart, compEnd - compStart + 1);

                                                        // Parse component type
                                                        size_t typePos = compStr.find("\"type\"");
                                                        size_t modelIDPos = compStr.find("\"modelID\"");
                                                        size_t fovPos = compStr.find("\"fov\"");
                                                        size_t nearPos = compStr.find("\"near\"");
                                                        size_t farPos = compStr.find("\"far\"");
                                                        size_t isMainCameraPos = compStr.find("\"isMainCamera\"");

                                                        if (typePos != std::string::npos) {
                                                            size_t colonPos = compStr.find(':', typePos);
                                                            size_t valueStart = compStr.find_first_not_of(" \t\n\r", colonPos + 1);
                                                            size_t valueEnd = compStr.find_first_of(",}\n\r", valueStart);
                                                            int componentType = 0;
                                                            if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                                                try {
                                                                    componentType = std::stoi(compStr.substr(valueStart, valueEnd - valueStart));
                                                                } catch (...) {}
                                                            }

                                                            // Create CameraComponent
                                                            // Editor uses type 3 for Camera, which maps to ComponentType::Camera (4)
                                                            // But we check for both 3 and 4 for compatibility
                                                            if (componentType == 3 || componentType == static_cast<int>(ComponentType::Camera)) {
                                                                CameraComponent* cameraComp = entity->AddComponent<CameraComponent>();
                                                                
                                                                // Parse FOV
                                                                if (fovPos != std::string::npos) {
                                                                    size_t colonPos2 = compStr.find(':', fovPos);
                                                                    size_t valueStart2 = compStr.find_first_not_of(" \t\n\r", colonPos2 + 1);
                                                                    size_t valueEnd2 = compStr.find_first_of(",}\n\r", valueStart2);
                                                                    if (valueStart2 != std::string::npos && valueEnd2 != std::string::npos) {
                                                                        try {
                                                                            float fov = std::stof(compStr.substr(valueStart2, valueEnd2 - valueStart2));
                                                                            cameraComp->SetFOV(fov);
                                                                        } catch (...) {}
                                                                    }
                                                                }
                                                                
                                                                // Parse Near
                                                                if (nearPos != std::string::npos) {
                                                                    size_t colonPos2 = compStr.find(':', nearPos);
                                                                    size_t valueStart2 = compStr.find_first_not_of(" \t\n\r", colonPos2 + 1);
                                                                    size_t valueEnd2 = compStr.find_first_of(",}\n\r", valueStart2);
                                                                    if (valueStart2 != std::string::npos && valueEnd2 != std::string::npos) {
                                                                        try {
                                                                            float nearPlane = std::stof(compStr.substr(valueStart2, valueEnd2 - valueStart2));
                                                                            cameraComp->SetNear(nearPlane);
                                                                        } catch (...) {}
                                                                    }
                                                                }
                                                                
                                                                // Parse Far
                                                                if (farPos != std::string::npos) {
                                                                    size_t colonPos2 = compStr.find(':', farPos);
                                                                    size_t valueStart2 = compStr.find_first_not_of(" \t\n\r", colonPos2 + 1);
                                                                    size_t valueEnd2 = compStr.find_first_of(",}\n\r", valueStart2);
                                                                    if (valueStart2 != std::string::npos && valueEnd2 != std::string::npos) {
                                                                        try {
                                                                            float farPlane = std::stof(compStr.substr(valueStart2, valueEnd2 - valueStart2));
                                                                            cameraComp->SetFar(farPlane);
                                                                        } catch (...) {}
                                                                    }
                                                                }
                                                                
                                                                // Parse IsMainCamera
                                                                if (isMainCameraPos != std::string::npos) {
                                                                    size_t colonPos2 = compStr.find(':', isMainCameraPos);
                                                                    size_t valueStart2 = compStr.find_first_not_of(" \t\n\r", colonPos2 + 1);
                                                                    size_t valueEnd2 = compStr.find_first_of(",}\n\r", valueStart2);
                                                                    if (valueStart2 != std::string::npos && valueEnd2 != std::string::npos) {
                                                                        std::string boolStr = compStr.substr(valueStart2, valueEnd2 - valueStart2);
                                                                        // Remove quotes if present
                                                                        boolStr.erase(std::remove(boolStr.begin(), boolStr.end(), '\"'), boolStr.end());
                                                                        bool isMain = (boolStr == "true" || boolStr == "True" || boolStr == "1");
                                                                        cameraComp->SetIsMainCamera(isMain);
                                                                    }
                                                                }
                                                                
                                                                std::cout << "SceneLoader: Created CameraComponent for entity " << entity->GetName() 
                                                                          << " (FOV=" << cameraComp->GetFOV() 
                                                                          << ", Near=" << cameraComp->GetNear() 
                                                                          << ", Far=" << cameraComp->GetFar()
                                                                          << ", IsMain=" << (cameraComp->IsMainCamera() ? "true" : "false") << ")" << std::endl;
                                                            }
                                                            // Create ModelComponent
                                                            // Note: ModelComponent uses ComponentType::Mesh (not ComponentType::Model)
                                                            else if (componentType == static_cast<int>(ComponentType::Mesh) && modelIDPos != std::string::npos) {
                                                                size_t colonPos2 = compStr.find(':', modelIDPos);
                                                                size_t valueStart = compStr.find_first_not_of(" \t\n\r", colonPos2 + 1);
                                                                size_t valueEnd = compStr.find_first_of(",}\n\r", valueStart);
                                                                ResourceID modelID = 0;
                                                                if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                                                                    try {
                                                                        modelID = std::stoull(compStr.substr(valueStart, valueEnd - valueStart));
                                                                        std::cout << "SceneLoader: Loading model with ID " << modelID << " for entity " << entity->GetName() << std::endl;
                                                                        
                                                                        // Check if resource ID is valid
                                                                        if (modelID == 0 || modelID == InvalidResourceID) {
                                                                            std::cerr << "SceneLoader: Invalid model ID " << modelID << " for entity " << entity->GetName() << std::endl;
                                                                            continue; // Skip this component
                                                                        }
                                                                        
                                                                        // Check if resource path exists in manifest
                                                                        std::string modelPath = resourceManager.GetResolvedPath(modelID);
                                                                        if (modelPath.empty()) {
                                                                            std::cerr << "SceneLoader: Model ID " << modelID << " not found in resource manifest for entity " << entity->GetName() << std::endl;
                                                                            std::cerr << "  Make sure the model is registered in resource_manifest.json" << std::endl;
                                                                            continue; // Skip this component
                                                                        }
                                                                        
                                                                        // Load model resource through ResourceManager
                                                                        // ResourceManager internally creates ModelResource and calls ModelResource::Load
                                                                        // This ensures unified loading interface using ModelResource's Load method
                                                                        ResourceHandle modelHandle = resourceManager.Load(modelID);
                                                                        
                                                                        // Check if model loaded successfully
                                                                        // Note: ResourceHandle is a union, so ptr and model point to the same memory
                                                                        if (modelHandle.ptr != nullptr && modelHandle.model != nullptr) {
                                                                            // Verify model is valid (has at least one mesh)
                                                                            uint32_t meshCount = modelHandle.model->GetMeshCount();
                                                                            if (meshCount == 0) {
                                                                                std::cerr << "SceneLoader: Model ID " << modelID << " loaded but has no meshes for entity " << entity->GetName() << std::endl;
                                                                                std::cerr << "  Model path: " << modelPath << std::endl;
                                                                                std::cerr << "  This usually means the model's mesh dependencies failed to load" << std::endl;
                                                                                continue; // Skip this component
                                                                            }
                                                                            
                                                                            ModelComponent* modelComp = entity->AddComponent<ModelComponent>();
                                                                            if (modelComp) {
                                                                                modelComp->SetModel(modelHandle.model);
                                                                                std::cout << "SceneLoader: Successfully loaded model ID " << modelID << " with " << modelHandle.model->GetMeshCount() << " mesh(es) for entity " << entity->GetName() << std::endl;
                                                                            } else {
                                                                                std::cerr << "SceneLoader: Failed to create ModelComponent for entity " << entity->GetName() << std::endl;
                                                                            }
                                                                        } else {
                                                                            std::cerr << "SceneLoader: Failed to load model ID " << modelID << " for entity " << entity->GetName() << std::endl;
                                                                            std::cerr << "  Model path: " << modelPath << std::endl;
                                                                            std::cerr << "  Check if the model file exists and is valid" << std::endl;
                                                                            // Don't create ModelComponent if model failed to load
                                                                        }
                                                                    } catch (const std::exception& e) {
                                                                        std::cerr << "SceneLoader: Exception loading model ID " << modelID << " for entity " << entity->GetName() << ": " << e.what() << std::endl;
                                                                    } catch (...) {
                                                                        std::cerr << "SceneLoader: Unknown exception loading model ID " << modelID << " for entity " << entity->GetName() << std::endl;
                                                                    }
                                                                } else {
                                                                    std::cerr << "SceneLoader: Failed to parse modelID from component string for entity " << entity->GetName() << std::endl;
                                                                }
                                                            }
                                                        }

                                                        compPos = compEnd + 1;
                                                        size_t nextComp = entityStr.find('{', compPos);
                                                        if (nextComp == std::string::npos || nextComp > entityStr.find(']', componentsArrayStart)) {
                                                            break;
                                                        }
                                                    }
                                                }
                                            }

                                            // Entity is fully loaded (all components attached), call OnLoad()
                                            entity->OnLoad();

                                            entityPos = entityEnd + 1;
                                            size_t nextEntity = levelStr.find('{', entityPos);
                                            if (nextEntity == std::string::npos || nextEntity > levelStr.find(']', entitiesArrayStart)) {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            pos = levelEnd + 1;
                            size_t nextLevel = content.find('{', pos);
                            if (nextLevel == std::string::npos || nextLevel > content.find(']', arrayStart)) {
                                break;
                            }
                        }
                    }
                }

                return true;
            } catch (...) {
                return false;
            }
        }

    } // namespace Resources
} // namespace FirstEngine
