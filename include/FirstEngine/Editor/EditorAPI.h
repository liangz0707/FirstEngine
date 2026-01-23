#pragma once

#include "FirstEngine/Core/Export.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct RenderViewport;

// Engine initialization and shutdown
// Note: Now returns void*, actually RenderContext*, but C# doesn't need to know the specific type
FE_CORE_API void* EditorAPI_CreateEngine(void* windowHandle, int width, int height);
FE_CORE_API void EditorAPI_DestroyEngine(void* engine);
FE_CORE_API bool EditorAPI_InitializeEngine(void* engine);
FE_CORE_API void EditorAPI_ShutdownEngine(void* engine);

// Render viewport management
// Note: engine parameter is now only used for validation, actually uses global RenderContext
FE_CORE_API RenderViewport* EditorAPI_CreateViewport(void* engine, void* parentWindowHandle, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_DestroyViewport(RenderViewport* viewport);
FE_CORE_API void EditorAPI_ResizeViewport(RenderViewport* viewport, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_SetViewportActive(RenderViewport* viewport, bool active);
FE_CORE_API void* EditorAPI_GetViewportWindowHandle(RenderViewport* viewport); // Get HWND of viewport window

// Frame rendering
// Corresponds to RenderApp::OnPrepareFrameGraph() - Build FrameGraph
// Note: engine parameter is now only used for validation, actually uses global RenderContext
FE_CORE_API void EditorAPI_PrepareFrameGraph(void* engine);
// Corresponds to RenderApp::OnCreateResources() - Process resources
FE_CORE_API void EditorAPI_CreateResources(void* engine);
// Corresponds to RenderApp::OnRender() - Execute FrameGraph to generate render commands
FE_CORE_API void EditorAPI_Render(void* engine);
// Corresponds to RenderApp::OnSubmit() - Submit rendering
FE_CORE_API void EditorAPI_SubmitFrame(RenderViewport* viewport);
// Deprecated: Use EditorAPI_PrepareFrameGraph + EditorAPI_CreateResources + EditorAPI_Render instead
FE_CORE_API void EditorAPI_BeginFrame(void* engine);
// Deprecated: No longer needed
FE_CORE_API void EditorAPI_EndFrame(void* engine);
// Deprecated: Use EditorAPI_SubmitFrame instead
FE_CORE_API void EditorAPI_RenderViewport(RenderViewport* viewport);
FE_CORE_API bool EditorAPI_IsViewportReady(RenderViewport* viewport);

// Scene management
// Note: engine parameter is now only used for validation, actually uses global RenderContext
FE_CORE_API void EditorAPI_LoadScene(void* engine, const char* scenePath);
FE_CORE_API void EditorAPI_UnloadScene(void* engine);
FE_CORE_API bool EditorAPI_SaveScene(void* engine, const char* scenePath);

// Resource management
FE_CORE_API void EditorAPI_LoadResource(void* engine, const char* resourcePath, int resourceType);
FE_CORE_API void EditorAPI_UnloadResource(void* engine, const char* resourcePath);

// Configuration
FE_CORE_API void EditorAPI_SetRenderConfig(void* engine, int width, int height, float renderScale);

// Console output redirection
// Callback function type: void callback(const char* message, int isError)
// message: The output message
// isError: 1 if from std::cerr, 0 if from std::cout
// callback: Function pointer (void*) - will be cast to OutputCallback internally
FE_CORE_API void EditorAPI_SetOutputCallback(void* callback);

// Enable/disable console output redirection to callback
FE_CORE_API void EditorAPI_EnableConsoleRedirect(int enable);  // Use int instead of bool for C compatibility

// Allocate/Free console window (Windows only)
FE_CORE_API void EditorAPI_AllocConsole();
FE_CORE_API void EditorAPI_FreeConsole();

#ifdef __cplusplus
}
#endif
