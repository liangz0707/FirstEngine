#pragma once

#include "FirstEngine/Core/Export.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct RenderEngine;
struct RenderViewport;

// Engine initialization and shutdown
FE_CORE_API RenderEngine* EditorAPI_CreateEngine(void* windowHandle, int width, int height);
FE_CORE_API void EditorAPI_DestroyEngine(RenderEngine* engine);
FE_CORE_API bool EditorAPI_InitializeEngine(RenderEngine* engine);
FE_CORE_API void EditorAPI_ShutdownEngine(RenderEngine* engine);

// Render viewport management
FE_CORE_API RenderViewport* EditorAPI_CreateViewport(RenderEngine* engine, void* parentWindowHandle, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_DestroyViewport(RenderViewport* viewport);
FE_CORE_API void EditorAPI_ResizeViewport(RenderViewport* viewport, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_SetViewportActive(RenderViewport* viewport, bool active);
FE_CORE_API void* EditorAPI_GetViewportWindowHandle(RenderViewport* viewport); // Get HWND of viewport window

// Frame rendering
FE_CORE_API void EditorAPI_BeginFrame(RenderEngine* engine);
FE_CORE_API void EditorAPI_EndFrame(RenderEngine* engine);
FE_CORE_API void EditorAPI_RenderViewport(RenderViewport* viewport);
FE_CORE_API bool EditorAPI_IsViewportReady(RenderViewport* viewport);

// Scene management
FE_CORE_API void EditorAPI_LoadScene(RenderEngine* engine, const char* scenePath);
FE_CORE_API void EditorAPI_UnloadScene(RenderEngine* engine);

// Resource management
FE_CORE_API void EditorAPI_LoadResource(RenderEngine* engine, const char* resourcePath, int resourceType);
FE_CORE_API void EditorAPI_UnloadResource(RenderEngine* engine, const char* resourcePath);

// Configuration
FE_CORE_API void EditorAPI_SetRenderConfig(RenderEngine* engine, int width, int height, float renderScale);

#ifdef __cplusplus
}
#endif
