#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <cstdint>

namespace FirstEngine {
    namespace Renderer {

        // Render object flags - used to filter which objects to render in each pass
        enum class RenderObjectFlag : uint32_t {
            None = 0,
            Opaque = 1 << 0,        // Opaque objects
            Transparent = 1 << 1,    // Transparent objects
            Shadow = 1 << 2,         // Objects that cast shadows
            UI = 1 << 3,             // UI elements
            Skybox = 1 << 4,         // Skybox
            Decal = 1 << 5,          // Decals
            All = 0xFFFFFFFF         // All objects
        };

        // Bitwise operators for RenderObjectFlag
        inline RenderObjectFlag operator|(RenderObjectFlag a, RenderObjectFlag b) {
            return static_cast<RenderObjectFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        inline RenderObjectFlag operator&(RenderObjectFlag a, RenderObjectFlag b) {
            return static_cast<RenderObjectFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
        }

        inline RenderObjectFlag& operator|=(RenderObjectFlag& a, RenderObjectFlag b) {
            a = a | b;
            return a;
        }

        inline RenderObjectFlag& operator&=(RenderObjectFlag& a, RenderObjectFlag b) {
            a = a & b;
            return a;
        }

    } // namespace Renderer
} // namespace FirstEngine
