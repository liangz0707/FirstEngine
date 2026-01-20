#include "FirstEngine/Editor/EditorAPI.h"
#include "FirstEngine/Core/Application.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Core/RenderDoc.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/CommandRecorder.h"
#include "FirstEngine/Renderer/RenderContext.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ISwapchain.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// 全局渲染引擎管理器（单例模式）
// RenderContext 现在管理完整的渲染引擎状态，不再需要单独的 RenderEngine 结构
static FirstEngine::Renderer::RenderContext* g_GlobalRenderContext = nullptr;
static std::mutex g_RenderContextMutex;

// Viewport 结构（简化，不再需要 engine 指针）
struct RenderViewport {
    void* parentWindowHandle = nullptr;
    void* childWindowHandle = nullptr;  // Win32 HWND for child window
    FirstEngine::RHI::ISwapchain* swapchain = nullptr;
    int x = 0, y = 0;
    int width = 0, height = 0;
    bool active = false;
    bool ready = false;
    
#ifdef _WIN32
    static LRESULT CALLBACK ViewportWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // Get viewport from window user data
        RenderViewport* viewport = reinterpret_cast<RenderViewport*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        switch (uMsg) {
            case WM_SIZE:
                if (viewport) {
                    // Update viewport size
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    viewport->width = rect.right - rect.left;
                    viewport->height = rect.bottom - rect.top;
                    
                    // Recreate swapchain if needed
                    if (viewport->swapchain && viewport->width > 0 && viewport->height > 0) {
                        viewport->swapchain->Recreate();
                    }
                }
                return 0;
            case WM_DESTROY:
                return 0;
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }
#endif
};


// Engine management
extern "C" {

// 创建引擎（现在只是初始化全局 RenderContext）
void* EditorAPI_CreateEngine(void* windowHandle, int width, int height) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (g_GlobalRenderContext) {
        // 已经存在，返回现有实例
        return g_GlobalRenderContext;
    }
    
    // 创建全局 RenderContext
    g_GlobalRenderContext = new FirstEngine::Renderer::RenderContext();
    return g_GlobalRenderContext;
}

// 销毁引擎
void EditorAPI_DestroyEngine(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (g_GlobalRenderContext) {
        delete g_GlobalRenderContext;
        g_GlobalRenderContext = nullptr;
    }
}

// 初始化引擎
bool EditorAPI_InitializeEngine(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return false;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (context->IsEngineInitialized()) {
        return true;
    }
    
    // Initialize RenderDoc BEFORE creating Vulkan instance
    // This is critical - RenderDoc needs to intercept Vulkan calls from the start
#ifdef _WIN32
    FirstEngine::Core::RenderDocHelper::Initialize();
#endif
    
    // 从 context 获取窗口句柄（如果有的话，否则使用 nullptr）
    void* windowHandle = nullptr;  // EditorAPI_CreateEngine 时传入的，但我们现在不需要它
    int width = 1920, height = 1080;  // 默认值，实际会通过 SetRenderConfig 设置
    
    return context->InitializeEngine(windowHandle, width, height);
}

void EditorAPI_ShutdownEngine(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    context->ShutdownEngine();
}

// Viewport management
RenderViewport* EditorAPI_CreateViewport(void* engine, void* parentWindowHandle, int x, int y, int width, int height) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return nullptr;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return nullptr;
    }
    
    auto* viewport = new RenderViewport();
    viewport->parentWindowHandle = parentWindowHandle;
    viewport->x = x;
    viewport->y = y;
    viewport->width = width;
    viewport->height = height;
    viewport->active = true;
    
#ifdef _WIN32
    // Create child window for embedding
    HWND hwndParent = reinterpret_cast<HWND>(parentWindowHandle);
    if (!hwndParent) {
        delete viewport;
        return nullptr;
    }
    
    // Register window class for viewport (only once)
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = RenderViewport::ViewportWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"FirstEngineViewport";
        
        if (RegisterClassExW(&wc)) {
            classRegistered = true;
        } else {
            std::cerr << "Failed to register viewport window class" << std::endl;
            delete viewport;
            return nullptr;
        }
    }
    
    // Create child window
    HWND hwndChild = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"FirstEngineViewport",
        L"RenderViewport",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        x, y, width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwndChild) {
        std::cerr << "Failed to create viewport child window" << std::endl;
        delete viewport;
        return nullptr;
    }
    
    // Store viewport pointer in window user data
    SetWindowLongPtr(hwndChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(viewport));
    viewport->childWindowHandle = hwndChild;
    
    // Create swapchain for viewport
    // Note: Currently CreateSwapchain uses the main window's surface
    // For proper viewport embedding, we would need to create a separate surface for the child window
    // This is a limitation that can be addressed later by refactoring swapchain creation
    FirstEngine::RHI::SwapchainDescription swapchainDesc;
    swapchainDesc.width = width;
    swapchainDesc.height = height;
    
    // Get device from global RenderContext
    auto* device = context->GetDevice();
    if (!device) {
        DestroyWindow(hwndChild);
        delete viewport;
        return nullptr;
    }
    
    // For now, use parent window handle (swapchain will render to main window)
    // TODO: Create separate surface and swapchain for child window
    auto swapchainPtr = device->CreateSwapchain(hwndParent, swapchainDesc);
    viewport->swapchain = swapchainPtr.release();
    
    if (viewport->swapchain) {
        viewport->ready = true;
    } else {
        std::cerr << "Failed to create swapchain for viewport" << std::endl;
        DestroyWindow(hwndChild);
        delete viewport;
        return nullptr;
    }
#else
    // Non-Windows: use parent window handle directly
    FirstEngine::RHI::SwapchainDescription swapchainDesc;
    swapchainDesc.width = width;
    swapchainDesc.height = height;
    
    auto* device = context->GetDevice();
    if (!device) {
        delete viewport;
        return nullptr;
    }
    
    auto swapchainPtr = device->CreateSwapchain(parentWindowHandle, swapchainDesc);
    viewport->swapchain = swapchainPtr.release();
    
    if (viewport->swapchain) {
        viewport->ready = true;
    }
#endif
    
    return viewport;
}

void EditorAPI_DestroyViewport(RenderViewport* viewport) {
    if (!viewport) return;
    
    if (viewport->swapchain) {
        // Swapchain cleanup is handled by device
        // Note: We don't delete it here as it's managed by unique_ptr in device
        viewport->swapchain = nullptr;
    }
    
#ifdef _WIN32
    if (viewport->childWindowHandle) {
        HWND hwnd = reinterpret_cast<HWND>(viewport->childWindowHandle);
        // Clear user data before destroying
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        DestroyWindow(hwnd);
        viewport->childWindowHandle = nullptr;
    }
#endif
    
    delete viewport;
}

void EditorAPI_ResizeViewport(RenderViewport* viewport, int x, int y, int width, int height) {
    if (!viewport) return;
    
    viewport->x = x;
    viewport->y = y;
    viewport->width = width;
    viewport->height = height;
    
#ifdef _WIN32
    if (viewport->childWindowHandle) {
        HWND hwnd = reinterpret_cast<HWND>(viewport->childWindowHandle);
        UINT SWP_NOZORDER_LOCAL = 0x0004;
        UINT SWP_NOACTIVATE_LOCAL = 0x0010;
        SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER_LOCAL | SWP_NOACTIVATE_LOCAL);
        
        // Update swapchain if size changed
        if (viewport->swapchain && width > 0 && height > 0) {
            viewport->swapchain->Recreate();
        }
        
        // Update render config in global RenderContext
        std::lock_guard<std::mutex> lock(g_RenderContextMutex);
        if (g_GlobalRenderContext && g_GlobalRenderContext->IsEngineInitialized()) {
            g_GlobalRenderContext->SetRenderConfig(width, height, 1.0f);
        }
    }
#else
    if (viewport->swapchain && width > 0 && height > 0) {
        viewport->swapchain->Recreate();
    }
    
    // Update render config in global RenderContext
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    if (g_GlobalRenderContext && g_GlobalRenderContext->IsEngineInitialized()) {
        g_GlobalRenderContext->SetRenderConfig(width, height, 1.0f);
    }
#endif
}

void EditorAPI_SetViewportActive(RenderViewport* viewport, bool active) {
    if (!viewport) return;
    viewport->active = active;
}

// Frame rendering
// 对应 RenderApp::OnPrepareFrameGraph() - 构建 FrameGraph
void EditorAPI_PrepareFrameGraph(void* engine) {
    // Debug: 添加输出以验证函数被调用
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph called" << std::endl;
    
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        std::cerr << "[EditorAPI] EditorAPI_PrepareFrameGraph: Invalid engine handle" << std::endl;
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        std::cerr << "[EditorAPI] EditorAPI_PrepareFrameGraph: Engine not initialized" << std::endl;
        return;
    }
    
    // Begin RenderDoc frame capture
#ifdef _WIN32
    FirstEngine::Core::RenderDocHelper::BeginFrame();
#endif
    
    // Debug: 添加输出以验证执行路径
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph: Calling BeginFrame" << std::endl;
    
    // 使用 RenderContext 构建 FrameGraph（所有参数都从内部获取）
    if (!context->BeginFrame()) {
        std::cerr << "Failed to prepare FrameGraph in RenderContext (editor)!" << std::endl;
        return;
    }
    
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph: BeginFrame completed" << std::endl;
}

// 对应 RenderApp::OnCreateResources() - 处理资源
void EditorAPI_CreateResources(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return;
    }

    // 处理计划中的资源
    context->ProcessResources(context->GetDevice(), 0);
}

// 对应 RenderApp::OnRender() - 执行 FrameGraph 生成渲染命令
void EditorAPI_Render(void* engine) {
    // Debug: 添加输出以验证函数被调用
    std::cout << "[EditorAPI] EditorAPI_Render called" << std::endl;
    
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        std::cerr << "[EditorAPI] EditorAPI_Render: Invalid engine handle" << std::endl;
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        std::cerr << "[EditorAPI] EditorAPI_Render: Engine not initialized" << std::endl;
        return;
    }

    // Debug: 添加输出以验证执行路径
    std::cout << "[EditorAPI] EditorAPI_Render: Calling ExecuteFrameGraph" << std::endl;
    
    // 使用 RenderContext 执行 FrameGraph（所有参数都从内部获取）
    if (!context->ExecuteFrameGraph()) {
        std::cerr << "Failed to execute FrameGraph in RenderContext (editor)!" << std::endl;
        return;
    }
    
    std::cout << "[EditorAPI] EditorAPI_Render: ExecuteFrameGraph completed" << std::endl;
}

// 对应 RenderApp::OnSubmit() - 提交渲染
void EditorAPI_SubmitFrame(RenderViewport* viewport) {
    // Debug: 添加输出以验证函数被调用
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame called" << std::endl;
    
    if (!viewport || !viewport->active || !viewport->ready) {
        std::cerr << "[EditorAPI] EditorAPI_SubmitFrame: Viewport invalid or not ready" << std::endl;
        return;
    }
    if (!viewport->swapchain) {
        std::cerr << "[EditorAPI] EditorAPI_SubmitFrame: No swapchain" << std::endl;
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!g_GlobalRenderContext || !g_GlobalRenderContext->IsEngineInitialized()) {
        std::cerr << "[EditorAPI] EditorAPI_SubmitFrame: Global context not initialized" << std::endl;
        return;
    }
    
    auto* context = g_GlobalRenderContext;
    
    // Debug: 添加输出以验证执行路径
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame: Calling SubmitFrame" << std::endl;
    
    // 使用 RenderContext 提交渲染（只需要提供 swapchain，其他都从内部获取）
    FirstEngine::Renderer::RenderContext::RenderParams params;
    params.swapchain = viewport->swapchain;
    
    if (!context->SubmitFrame(params)) {
        // 提交失败（可能是图像获取失败或需要重建 swapchain）
        std::cerr << "[EditorAPI] EditorAPI_SubmitFrame: SubmitFrame failed" << std::endl;
        return;
    }
    
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame: SubmitFrame completed" << std::endl;
    
    // End RenderDoc frame capture (after frame is submitted)
#ifdef _WIN32
    FirstEngine::Core::RenderDocHelper::EndFrame();
#endif
}

// 已废弃：为了向后兼容保留，内部调用新的方法
void EditorAPI_BeginFrame(void* engine) {
    // 调用新的分离方法
    EditorAPI_PrepareFrameGraph(engine);
    EditorAPI_CreateResources(engine);
    EditorAPI_Render(engine);
}

// 已废弃：为了向后兼容保留
void EditorAPI_EndFrame(void* engine) {
    // Frame end logic - command buffer will be released at start of next frame
}

// 已废弃：为了向后兼容保留，内部调用 EditorAPI_SubmitFrame
void EditorAPI_RenderViewport(RenderViewport* viewport) {
    EditorAPI_SubmitFrame(viewport);
}

bool EditorAPI_IsViewportReady(RenderViewport* viewport) {
    return viewport && viewport->ready;
}

void* EditorAPI_GetViewportWindowHandle(RenderViewport* viewport) {
    if (!viewport) return nullptr;
#ifdef _WIN32
    return viewport->childWindowHandle ? viewport->childWindowHandle : viewport->parentWindowHandle;
#else
    return viewport->parentWindowHandle;
#endif
}

// Scene management
void EditorAPI_LoadScene(void* engine, const char* scenePath) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine || !scenePath) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return;
    }
    
    context->LoadScene(scenePath);
}

void EditorAPI_UnloadScene(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return;
    }
    
    context->UnloadScene();
}

// Resource management
void EditorAPI_LoadResource(void* engine, const char* resourcePath, int resourceType) {
    // TODO: 实现资源加载逻辑
    (void)engine;
    (void)resourcePath;
    (void)resourceType;
}

void EditorAPI_UnloadResource(void* engine, const char* resourcePath) {
    // TODO: 实现资源卸载逻辑
    (void)engine;
    (void)resourcePath;
}

// Configuration
void EditorAPI_SetRenderConfig(void* engine, int width, int height, float renderScale) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return;
    }
    
    context->SetRenderConfig(width, height, renderScale);
}

} // extern "C"
