#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include <string>

namespace FirstEngine {
    namespace Resources {

        // Effect/Particle system component
        class FE_RESOURCES_API EffectComponent : public Component {
        public:
            EffectComponent();
            ~EffectComponent() override = default;

            void SetEffectPath(const std::string& path) { m_EffectPath = path; }
            const std::string& GetEffectPath() const { return m_EffectPath; }

            void SetEnabled(bool enabled) { m_Enabled = enabled; }
            bool IsEnabled() const { return m_Enabled; }

            void SetLooping(bool looping) { m_Looping = looping; }
            bool IsLooping() const { return m_Looping; }

            void SetDuration(float duration) { m_Duration = duration; }
            float GetDuration() const { return m_Duration; }

        private:
            std::string m_EffectPath;
            bool m_Enabled = true;
            bool m_Looping = false;
            float m_Duration = 0.0f; // 0 = infinite
        };

    } // namespace Resources
} // namespace FirstEngine
