#pragma once

#include "FirstEngine/Core/Export.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct RenderViewport;

// Engine initialization and shutdown
// 注意：现在返回 void*，实际是 RenderContext*，但 C# 不需要知道具体类型
FE_CORE_API void* EditorAPI_CreateEngine(void* windowHandle, int width, int height);
FE_CORE_API void EditorAPI_DestroyEngine(void* engine);
FE_CORE_API bool EditorAPI_InitializeEngine(void* engine);
FE_CORE_API void EditorAPI_ShutdownEngine(void* engine);

// Render viewport management
// 注意：engine 参数现在只是用于验证，实际使用全局 RenderContext
FE_CORE_API RenderViewport* EditorAPI_CreateViewport(void* engine, void* parentWindowHandle, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_DestroyViewport(RenderViewport* viewport);
FE_CORE_API void EditorAPI_ResizeViewport(RenderViewport* viewport, int x, int y, int width, int height);
FE_CORE_API void EditorAPI_SetViewportActive(RenderViewport* viewport, bool active);
FE_CORE_API void* EditorAPI_GetViewportWindowHandle(RenderViewport* viewport); // Get HWND of viewport window

// Frame rendering
// 对应 RenderApp::OnPrepareFrameGraph() - 构建 FrameGraph
// 注意：engine 参数现在只是用于验证，实际使用全局 RenderContext
FE_CORE_API void EditorAPI_PrepareFrameGraph(void* engine);
// 对应 RenderApp::OnCreateResources() - 处理资源
FE_CORE_API void EditorAPI_CreateResources(void* engine);
// 对应 RenderApp::OnRender() - 执行 FrameGraph 生成渲染命令
FE_CORE_API void EditorAPI_Render(void* engine);
// 对应 RenderApp::OnSubmit() - 提交渲染
FE_CORE_API void EditorAPI_SubmitFrame(RenderViewport* viewport);
// 已废弃：使用 EditorAPI_PrepareFrameGraph + EditorAPI_CreateResources + EditorAPI_Render 替代
FE_CORE_API void EditorAPI_BeginFrame(void* engine);
// 已废弃：不再需要
FE_CORE_API void EditorAPI_EndFrame(void* engine);
// 已废弃：使用 EditorAPI_SubmitFrame 替代
FE_CORE_API void EditorAPI_RenderViewport(RenderViewport* viewport);
FE_CORE_API bool EditorAPI_IsViewportReady(RenderViewport* viewport);

// Scene management
// 注意：engine 参数现在只是用于验证，实际使用全局 RenderContext
FE_CORE_API void EditorAPI_LoadScene(void* engine, const char* scenePath);
FE_CORE_API void EditorAPI_UnloadScene(void* engine);

// Resource management
FE_CORE_API void EditorAPI_LoadResource(void* engine, const char* resourcePath, int resourceType);
FE_CORE_API void EditorAPI_UnloadResource(void* engine, const char* resourcePath);

// Configuration
FE_CORE_API void EditorAPI_SetRenderConfig(void* engine, int width, int height, float renderScale);

#ifdef __cplusplus
}
#endif
