#pragma once

#include "FirstEngine/Resources/Export.h"
#include <cstdint>

namespace FirstEngine {
    namespace Resources {

        // Resource type enumeration
        enum class ResourceType : uint32_t {
            Unknown = 0,
            Mesh = 1,
            Material = 2,
            Texture = 3,
            Model = 4,
            Shader = 5
        };

    } // namespace Resources
} // namespace FirstEngine
