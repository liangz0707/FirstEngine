#pragma once

#include "FirstEngine/Renderer/Export.h"

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class MeshResource;
        class MaterialResource;
        // MeshHandle and MaterialHandle are defined in ResourceTypes.h
        // Don't redefine here to avoid conflicts
    }
}

namespace FirstEngine {
    namespace Renderer {
        // RenderResource.h - forward declarations only
        // RenderMesh has been removed - use RenderGeometry instead
        // RenderGeometry is an IRenderResource and provides the same functionality
    } // namespace Renderer
} // namespace FirstEngine
