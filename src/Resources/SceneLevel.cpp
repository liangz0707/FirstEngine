#include "FirstEngine/Resources/SceneLevel.h"
#include "FirstEngine/Resources/Scene.h"
#include <algorithm>

namespace FirstEngine {
    namespace Resources {

        SceneLevel::SceneLevel(Scene* scene, const std::string& name, uint32_t order)
            : m_Scene(scene), m_Name(name), m_Order(order) {}

        SceneLevel::~SceneLevel() {
            // Entities are owned by Scene, not SceneLevel
            m_Entities.clear();
        }

        Entity* SceneLevel::CreateEntity(const std::string& name) {
            if (!m_Scene) return nullptr;
            // Scene::CreateEntity will automatically add to this level
            return m_Scene->CreateEntity(name, m_Name);
        }

        void SceneLevel::AddEntity(Entity* entity) {
            if (!entity) return;
            m_Entities.push_back(entity);
            m_EntityMap[entity->GetID()] = entity;
            if (!entity->GetName().empty()) {
                m_EntityNameMap[entity->GetName()] = entity;
            }
        }

        void SceneLevel::RemoveEntity(Entity* entity) {
            if (!entity) return;
            RemoveEntity(entity->GetID());
        }

        void SceneLevel::RemoveEntity(uint64_t id) {
            auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
                [id](Entity* e) { return e->GetID() == id; });
            if (it != m_Entities.end()) {
                Entity* entity = *it;
                m_EntityMap.erase(id);
                if (!entity->GetName().empty()) {
                    m_EntityNameMap.erase(entity->GetName());
                }
                m_Entities.erase(it);
            }
        }

        Entity* SceneLevel::GetEntity(uint64_t id) const {
            auto it = m_EntityMap.find(id);
            return it != m_EntityMap.end() ? it->second : nullptr;
        }

        Entity* SceneLevel::FindEntityByName(const std::string& name) const {
            auto it = m_EntityNameMap.find(name);
            return it != m_EntityNameMap.end() ? it->second : nullptr;
        }

    } // namespace Resources
} // namespace FirstEngine
