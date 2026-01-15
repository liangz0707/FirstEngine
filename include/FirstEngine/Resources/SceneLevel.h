#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Scene.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        class Scene;
        class Entity;

        // Scene level - represents a rendering layer in the scene
        class FE_RESOURCES_API SceneLevel {
        public:
            SceneLevel(Scene* scene, const std::string& name, uint32_t order = 0);
            ~SceneLevel();

            const std::string& GetName() const { return m_Name; }
            void SetName(const std::string& name) { m_Name = name; }

            uint32_t GetOrder() const { return m_Order; }
            void SetOrder(uint32_t order) { m_Order = order; }

            Scene* GetScene() const { return m_Scene; }

            // Entity management
            Entity* CreateEntity(const std::string& name = "");
            void AddEntity(Entity* entity);
            void RemoveEntity(Entity* entity);
            void RemoveEntity(uint64_t id);

            Entity* GetEntity(uint64_t id) const;
            Entity* FindEntityByName(const std::string& name) const;

            const std::vector<Entity*>& GetEntities() const { return m_Entities; }

            // Visibility
            void SetVisible(bool visible) { m_Visible = visible; }
            bool IsVisible() const { return m_Visible; }

            // Enabled state
            void SetEnabled(bool enabled) { m_Enabled = enabled; }
            bool IsEnabled() const { return m_Enabled; }

        private:
            Scene* m_Scene;
            std::string m_Name;
            uint32_t m_Order; // Rendering order (lower values render first)
            bool m_Visible = true;
            bool m_Enabled = true;
            std::vector<Entity*> m_Entities;
            std::unordered_map<uint64_t, Entity*> m_EntityMap;
            std::unordered_map<std::string, Entity*> m_EntityNameMap;
        };

    } // namespace Resources
} // namespace FirstEngine
