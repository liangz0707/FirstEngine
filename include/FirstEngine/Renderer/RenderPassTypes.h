#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <string>

namespace FirstEngine {
    namespace Renderer {

        // Render Pass Type enumeration
        // Used to identify the type of a render pass node
        enum class FE_RENDERER_API RenderPassType {
            Unknown,
            Geometry,      // Geometry pass (G-Buffer generation)
            Lighting,      // Lighting pass (deferred lighting calculation)
            Forward,       // Forward rendering pass
            PostProcess,   // Post-processing pass
            Shadow,        // Shadow map generation pass
            Skybox,        // Skybox rendering pass
            UI,            // UI rendering pass
            Custom         // Custom pass type
        };

        // Convert RenderPassType to string (for debugging/logging)
        FE_RENDERER_API std::string RenderPassTypeToString(RenderPassType type);

        // Convert string to RenderPassType
        FE_RENDERER_API RenderPassType StringToRenderPassType(const std::string& str);

        // FrameGraph Resource Name enumeration
        // Common resource names used across different render passes
        enum class FE_RENDERER_API FrameGraphResourceName {
            // G-Buffer resources
            GBufferAlbedo,
            GBufferNormal,
            GBufferMaterial,
            GBufferDepth,
            GBufferRoughness,
            GBufferMetallic,
            
            // Output resources
            FinalOutput,
            HDRBuffer,
            
            // Shadow resources
            ShadowMap,
            
            // Post-process resources
            PostProcessBuffer,
            
            // Custom resource (use string name)
            Custom
        };

        // Convert FrameGraphResourceName to string
        FE_RENDERER_API std::string FrameGraphResourceNameToString(FrameGraphResourceName name);

        // Convert string to FrameGraphResourceName
        FE_RENDERER_API FrameGraphResourceName StringToFrameGraphResourceName(const std::string& str);

    } // namespace Renderer
} // namespace FirstEngine
