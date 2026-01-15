#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Resources {

        // Model component - references a model resource and provides render data
        class FE_RESOURCES_API ModelComponent : public Component {
        public:
            ModelComponent();
            ~ModelComponent() override = default;

            // Resource management
            void SetModel(ModelHandle model);
            ModelHandle GetModel() const { return m_Model; }

            // Bounds
            AABB GetBounds() const override;

        private:
            ModelHandle m_Model = nullptr;
        };

    } // namespace Resources
} // namespace FirstEngine
