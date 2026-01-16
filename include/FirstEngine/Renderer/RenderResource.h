#pragma once

#include "FirstEngine/Renderer/Export.h"

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class MeshResource;
        class MaterialResource;
        using MeshHandle = MeshResource*;
        using MaterialHandle = MaterialResource*;
    }
}

namespace FirstEngine {
    namespace Renderer {
        // RenderResource.h - forward declarations only
        // RenderMesh has been removed - use RenderGeometry instead
        // RenderGeometry is an IRenderResource and provides the same functionality
    } // namespace Renderer
} // namespace FirstEngine
