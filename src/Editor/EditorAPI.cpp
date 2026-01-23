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
#include "FirstEngine/Resources/Scene.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <cstdio>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Global rendering engine manager (singleton pattern)
// RenderContext now manages complete rendering engine state, no longer needs separate RenderEngine structure
static FirstEngine::Renderer::RenderContext* g_GlobalRenderContext = nullptr;
static std::mutex g_RenderContextMutex;

#ifdef _WIN32
// Console output redirection
typedef void (*OutputCallback)(const char* message, int isError);

static OutputCallback g_OutputCallback = nullptr;
static std::mutex g_OutputMutex;

// Custom streambuf to redirect output to callback function
class CallbackStreamBuf : public std::streambuf {
public:
    CallbackStreamBuf(bool isError) : m_IsError(isError) {}

protected:
    virtual int_type overflow(int_type c) override {
        if (c != EOF) {
            if (c == '\n') {
                // When encountering newline, output the buffered line immediately
                // This handles the case where std::endl is used (which outputs \n and calls sync)
                std::string line = m_Buffer.str();
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(g_OutputMutex);
                    // Only send to callback (C# will format it), don't also send to OutputDebugStringA to avoid duplication
                    if (g_OutputCallback) {
                        g_OutputCallback(line.c_str(), m_IsError ? 1 : 0);
                    }
                    // OutputDebugStringA is called by C# callback if needed, so we don't need it here
                }
                m_Buffer.str("");
                m_Buffer.clear();
                m_LastWasNewline = true;  // Mark that we just processed a newline
            } else {
                m_Buffer << static_cast<char>(c);
                m_LastWasNewline = false;
            }
        }
        return c;
    }

    virtual int sync() override {
        // Only output if there's content in the buffer AND we didn't just process a newline
        // This prevents duplicate output when std::endl is used (which calls both overflow('\n') and sync())
        if (!m_LastWasNewline) {
            std::string line = m_Buffer.str();
            if (!line.empty()) {
                std::lock_guard<std::mutex> lock(g_OutputMutex);
                // Only send to callback (C# will format it), don't also send to OutputDebugStringA to avoid duplication
                if (g_OutputCallback) {
                    g_OutputCallback(line.c_str(), m_IsError ? 1 : 0);
                }
                // OutputDebugStringA is called by C# callback if needed, so we don't need it here
                m_Buffer.str("");
                m_Buffer.clear();
            }
        }
        m_LastWasNewline = false;  // Reset flag after sync
        return 0;
    }

private:
    std::stringstream m_Buffer;
    bool m_IsError;
    bool m_LastWasNewline = false;  // Track if we just processed a newline in overflow
};

static CallbackStreamBuf* g_CoutBuf = nullptr;
static CallbackStreamBuf* g_CerrBuf = nullptr;
static std::streambuf* g_OriginalCout = nullptr;
static std::streambuf* g_OriginalCerr = nullptr;
#endif

// Viewport structure (simplified, no longer needs engine pointer)
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

// Create engine (now just initializes global RenderContext)
void* EditorAPI_CreateEngine(void* windowHandle, int width, int height) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (g_GlobalRenderContext) {
        // Already exists, return existing instance
        return g_GlobalRenderContext;
    }
    
    // Create global RenderContext
    g_GlobalRenderContext = new FirstEngine::Renderer::RenderContext();
    return g_GlobalRenderContext;
}

// Destroy engine
void EditorAPI_DestroyEngine(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (g_GlobalRenderContext) {
        delete g_GlobalRenderContext;
        g_GlobalRenderContext = nullptr;
    }
}

// Initialize engine
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
    
    // Get window handle from context (if available, otherwise use nullptr)
    void* windowHandle = nullptr;  // Passed in EditorAPI_CreateEngine, but we don't need it now
    int width = 1920, height = 1080;  // Default values, will be set via SetRenderConfig
    
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
    
    // Ensure minimum size
    if (width <= 0) width = 800;
    if (height <= 0) height = 600;
    
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
    
    // Ensure window is properly sized and visible before creating surface
    // This is critical for vkGetPhysicalDeviceSurfaceCapabilitiesKHR to return correct size
    ShowWindow(hwndChild, SW_SHOW);
    UpdateWindow(hwndChild);
    
    // Force a layout update to ensure window has correct client area size
    RECT clientRect;
    GetClientRect(hwndChild, &clientRect);
    int actualWidth = clientRect.right - clientRect.left;
    int actualHeight = clientRect.bottom - clientRect.top;
    
    // If client area size doesn't match, adjust window size
    if (actualWidth != width || actualHeight != height) {
        RECT windowRect;
        GetWindowRect(hwndChild, &windowRect);
        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;
        
        // Calculate border size
        int borderWidth = windowWidth - actualWidth;
        int borderHeight = windowHeight - actualHeight;
        
        // Resize window to get correct client area
        SetWindowPos(hwndChild, nullptr, x, y, width + borderWidth, height + borderHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        
        // Verify again
        GetClientRect(hwndChild, &clientRect);
        actualWidth = clientRect.right - clientRect.left;
        actualHeight = clientRect.bottom - clientRect.top;
        
        std::cout << "Viewport window size adjusted: " << actualWidth << "x" << actualHeight << std::endl;
    }
    
    // Update viewport size to match actual window size
    viewport->width = actualWidth;
    viewport->height = actualHeight;
    
    // Create swapchain for viewport using child window handle
    // This will create a new surface for the child window with correct size
    FirstEngine::RHI::SwapchainDescription swapchainDesc;
    swapchainDesc.width = viewport->width;  // Use actual window size
    swapchainDesc.height = viewport->height;
    
    // Get device from global RenderContext
    auto* device = context->GetDevice();
    if (!device) {
        DestroyWindow(hwndChild);
        delete viewport;
        return nullptr;
    }
    
    // Use child window handle to create swapchain (this will create a surface for the child window)
    // This ensures the swapchain is created with the correct window size
    auto swapchainPtr = device->CreateSwapchain(hwndChild, swapchainDesc);
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
// Corresponds to RenderApp::OnPrepareFrameGraph() - Build FrameGraph
void EditorAPI_PrepareFrameGraph(void* engine) {
    // Debug: Add output to verify function is called
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph called" << std::endl;
#endif
    
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
    
    // Debug: Add output to verify execution path
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph: Calling BeginFrame" << std::endl;
#endif
    
    // Use RenderContext to build FrameGraph (all parameters are obtained internally)
    if (!context->BeginFrame()) {
        std::cerr << "Failed to prepare FrameGraph in RenderContext (editor)!" << std::endl;
        return;
    }
    
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_PrepareFrameGraph: BeginFrame completed" << std::endl;
#endif
}

// Corresponds to RenderApp::OnCreateResources() - Process resources
void EditorAPI_CreateResources(void* engine) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine) {
        return;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return;
    }

    // Process scheduled resources
    context->ProcessResources(context->GetDevice(), 0);
}

// Corresponds to RenderApp::OnRender() - Execute FrameGraph to generate render commands
void EditorAPI_Render(void* engine) {
    // Debug: Add output to verify function is called
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_Render called" << std::endl;
#endif
    
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

    // Debug: Add output to verify execution path
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_Render: Calling ExecuteFrameGraph" << std::endl;
#endif
    
    // Use RenderContext to execute FrameGraph (all parameters are obtained internally)
    if (!context->ExecuteFrameGraph()) {
        std::cerr << "Failed to execute FrameGraph in RenderContext (editor)!" << std::endl;
        return;
    }
    
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_Render: ExecuteFrameGraph completed" << std::endl;
#endif
}

// Corresponds to RenderApp::OnSubmit() - Submit rendering
void EditorAPI_SubmitFrame(RenderViewport* viewport) {
    // Debug: Add output to verify function is called
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame called" << std::endl;
#endif
    
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
    
    // Debug: Add output to verify execution path
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame: Calling SubmitFrame" << std::endl;
#endif
    
    // Use RenderContext to submit rendering (only need to provide swapchain, everything else is obtained internally)
    FirstEngine::Renderer::RenderContext::RenderParams params;
    params.swapchain = viewport->swapchain;
    
    if (!context->SubmitFrame(params)) {
        // Submit failed (may be image acquisition failure or need to recreate swapchain)
        std::cerr << "[EditorAPI] EditorAPI_SubmitFrame: SubmitFrame failed" << std::endl;
        return;
    }
    
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_SubmitFrame: SubmitFrame completed" << std::endl;
#endif
    
    // End RenderDoc frame capture (after frame is submitted)
#ifdef _WIN32
    FirstEngine::Core::RenderDocHelper::EndFrame();
#endif
}

// Deprecated: Kept for backward compatibility, internally calls new methods
void EditorAPI_BeginFrame(void* engine) {
    // Call new separated methods
    EditorAPI_PrepareFrameGraph(engine);
    EditorAPI_CreateResources(engine);
    EditorAPI_Render(engine);
}

// Deprecated: Kept for backward compatibility
void EditorAPI_EndFrame(void* engine) {
    // Frame end logic - command buffer will be released at start of next frame
}

// Deprecated: Kept for backward compatibility, internally calls EditorAPI_SubmitFrame
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

bool EditorAPI_SaveScene(void* engine, const char* scenePath) {
    std::lock_guard<std::mutex> lock(g_RenderContextMutex);
    
    if (!engine || g_GlobalRenderContext != engine || !scenePath) {
        return false;
    }
    
    auto* context = static_cast<FirstEngine::Renderer::RenderContext*>(engine);
    if (!context->IsEngineInitialized()) {
        return false;
    }
    
    // Get scene from RenderContext
    auto* scene = context->GetScene();
    if (!scene) {
        std::cerr << "[EditorAPI] EditorAPI_SaveScene: No scene loaded" << std::endl;
        return false;
    }
    
    // Save scene using SceneLoader
    return FirstEngine::Resources::SceneLoader::SaveToFile(scenePath, *scene);
}

// Resource management
void EditorAPI_LoadResource(void* engine, const char* resourcePath, int resourceType) {
    if (!resourcePath) {
        std::cerr << "[EditorAPI] EditorAPI_LoadResource: resourcePath is nullptr" << std::endl;
        return;
    }

    // Get ResourceManager instance
    auto& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();
    
    // Convert resourceType (int) to ResourceType enum
    FirstEngine::Resources::ResourceType type = static_cast<FirstEngine::Resources::ResourceType>(resourceType);
    
    // Validate resource type
    if (type == FirstEngine::Resources::ResourceType::Unknown) {
        // Try to detect type from file path
        type = resourceManager.DetectResourceType(resourcePath);
        if (type == FirstEngine::Resources::ResourceType::Unknown) {
            std::cerr << "[EditorAPI] EditorAPI_LoadResource: Unknown resource type for path: " << resourcePath << std::endl;
            return;
        }
    }
    
    // Load resource
    FirstEngine::Resources::ResourceHandle handle = resourceManager.Load(type, resourcePath);
    if (handle.ptr == nullptr) {
        std::cerr << "[EditorAPI] EditorAPI_LoadResource: Failed to load resource: " << resourcePath << std::endl;
    } else {
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
        std::cout << "[EditorAPI] EditorAPI_LoadResource: Successfully loaded resource: " << resourcePath << std::endl;
#endif
    }
    
    (void)engine; // Engine parameter kept for API compatibility
}

void EditorAPI_UnloadResource(void* engine, const char* resourcePath) {
    if (!resourcePath) {
        std::cerr << "[EditorAPI] EditorAPI_UnloadResource: resourcePath is nullptr" << std::endl;
        return;
    }

    // Get ResourceManager instance
    auto& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();
    
    // Try to detect resource type from path
    FirstEngine::Resources::ResourceType type = resourceManager.DetectResourceType(resourcePath);
    if (type == FirstEngine::Resources::ResourceType::Unknown) {
        std::cerr << "[EditorAPI] EditorAPI_UnloadResource: Unknown resource type for path: " << resourcePath << std::endl;
        return;
    }
    
    // Unload resource
    resourceManager.Unload(type, resourcePath);
    
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
    std::cout << "[EditorAPI] EditorAPI_UnloadResource: Unloaded resource: " << resourcePath << std::endl;
#endif
    
    (void)engine; // Engine parameter kept for API compatibility
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

// Console output redirection
#ifdef _WIN32
void EditorAPI_SetOutputCallback(void* callback) {
    std::lock_guard<std::mutex> lock(g_OutputMutex);
    g_OutputCallback = reinterpret_cast<OutputCallback>(callback);
}

void EditorAPI_EnableConsoleRedirect(int enable) {
    std::lock_guard<std::mutex> lock(g_OutputMutex);
    
    if (enable != 0) {
        if (!g_OriginalCout) {
            g_OriginalCout = std::cout.rdbuf();
        }
        if (!g_OriginalCerr) {
            g_OriginalCerr = std::cerr.rdbuf();
        }

        if (!g_CoutBuf) {
            g_CoutBuf = new CallbackStreamBuf(false);
        }
        if (!g_CerrBuf) {
            g_CerrBuf = new CallbackStreamBuf(true);
        }

        std::cout.rdbuf(g_CoutBuf);
        std::cerr.rdbuf(g_CerrBuf);
    } else {
        if (g_OriginalCout) {
            std::cout.rdbuf(g_OriginalCout);
            g_OriginalCout = nullptr;
        }
        if (g_OriginalCerr) {
            std::cerr.rdbuf(g_OriginalCerr);
            g_OriginalCerr = nullptr;
        }

        if (g_CoutBuf) {
            delete g_CoutBuf;
            g_CoutBuf = nullptr;
        }
        if (g_CerrBuf) {
            delete g_CerrBuf;
            g_CerrBuf = nullptr;
        }
    }
}

void EditorAPI_AllocConsole() {
    if (AllocConsole()) {
        FILE* pCout = nullptr;
        FILE* pCerr = nullptr;
        FILE* pCin = nullptr;
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        freopen_s(&pCin, "CONIN$", "r", stdin);
        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();
    }
}

void EditorAPI_FreeConsole() {
    FreeConsole();
}
#else
void EditorAPI_SetOutputCallback(void* callback) {
    (void)callback; // Unused
}

void EditorAPI_EnableConsoleRedirect(int enable) {
    (void)enable; // Unused
}

void EditorAPI_AllocConsole() {
}

void EditorAPI_FreeConsole() {
}
#endif

} // extern "C"
