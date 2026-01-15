#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        // Light types
        enum class LightType : uint32_t {
            Directional = 0,
            Point = 1,
            Spot = 2,
            Area = 3
        };

        // Light component
        class FE_RESOURCES_API LightComponent : public Component {
        public:
            LightComponent();
            ~LightComponent() override = default;

            void SetType(LightType type) { m_LightType = type; }
            LightType GetType() const { return m_LightType; }

            void SetColor(const glm::vec3& color) { m_Color = color; }
            const glm::vec3& GetColor() const { return m_Color; }

            void SetIntensity(float intensity) { m_Intensity = intensity; }
            float GetIntensity() const { return m_Intensity; }

            // For point/spot lights
            void SetRange(float range) { m_Range = range; }
            float GetRange() const { return m_Range; }

            // For spot lights
            void SetInnerConeAngle(float angle) { m_InnerConeAngle = angle; }
            float GetInnerConeAngle() const { return m_InnerConeAngle; }
            void SetOuterConeAngle(float angle) { m_OuterConeAngle = angle; }
            float GetOuterConeAngle() const { return m_OuterConeAngle; }

            // For area lights
            void SetSize(const glm::vec2& size) { m_Size = size; }
            const glm::vec2& GetSize() const { return m_Size; }

            // Shadow casting
            void SetCastShadows(bool cast) { m_CastShadows = cast; }
            bool GetCastShadows() const { return m_CastShadows; }

        private:
            LightType m_LightType = LightType::Point;
            glm::vec3 m_Color = glm::vec3(1.0f);
            float m_Intensity = 1.0f;
            float m_Range = 10.0f;
            float m_InnerConeAngle = 30.0f; // degrees
            float m_OuterConeAngle = 45.0f; // degrees
            glm::vec2 m_Size = glm::vec2(1.0f, 1.0f); // For area lights
            bool m_CastShadows = false;
        };

    } // namespace Resources
} // namespace FirstEngine
