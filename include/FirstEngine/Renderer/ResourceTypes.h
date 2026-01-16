#pragma once

#include "FirstEngine/Renderer/Export.h"

namespace FirstEngine {
    namespace Renderer {

        // FrameGraph resource type enum
        // This is defined in a separate header to avoid circular dependencies
        enum class ResourceType {
            Texture,
            Buffer,
            Attachment,
        };

    } // namespace Renderer
} // namespace FirstEngine
