#include "FirstEngine/Editor/EditorAPI.h"
#include "FirstEngine/Core/Application.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ISwapchain.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <iostream>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Internal structures
struct RenderEngine {
    FirstEngine::Device::VulkanDevice* device = nullptr;
    FirstEngine::Device::VulkanRenderer* renderer = nullptr;  // Access to Vulkan instance
    FirstEngine::Renderer::FrameGraph* frameGraph = nullptr;
    FirstEngine::Renderer::DeferredRenderPipeline* renderPipeline = nullptr;
    FirstEngine::Renderer::RenderConfig renderConfig;
    void* windowHandle = nullptr;
#ifdef _WIN32
    GLFWwindow* hiddenGLFWWindow = nullptr;  // Hidden GLFW window for Vulkan initialization
#endif
    bool initialized = false;
};

struct RenderViewport {
    RenderEngine* engine = nullptr;
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

RenderEngine* EditorAPI_CreateEngine(void* windowHandle, int width, int height) {
    auto* engine = new RenderEngine();
    engine->windowHandle = windowHandle;
    engine->renderConfig.SetResolution(width, height);
    return engine;
}

void EditorAPI_DestroyEngine(RenderEngine* engine) {
    if (!engine) return;
    
    EditorAPI_ShutdownEngine(engine);
    delete engine;
}

bool EditorAPI_InitializeEngine(RenderEngine* engine) {
    if (!engine || engine->initialized) return false;
    
    try {
        // Initialize singleton managers
        FirstEngine::Renderer::RenderResourceManager::Initialize();
        
        // Initialize ShaderCollectionsTools with shader directory
        auto& collectionsTools = FirstEngine::Renderer::ShaderCollectionsTools::GetInstance();
        if (!collectionsTools.Initialize("build/Package/Shaders")) {
            std::cerr << "Warning: Failed to initialize ShaderCollectionsTools, shaders may not be available." << std::endl;
        }
        
#ifdef _WIN32
        // For editor mode, we need a GLFW window for Vulkan initialization
        // Create a hidden GLFW window (VulkanDevice expects GLFWwindow*)
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW for editor" << std::endl;
            return false;
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Hidden window
        GLFWwindow* glfwWindow = glfwCreateWindow(1, 1, "FirstEngine Editor Hidden", nullptr, nullptr);
        
        if (!glfwWindow) {
            std::cerr << "Failed to create hidden GLFW window for editor" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Use GLFW window for initialization
        void* glfwHandle = static_cast<void*>(glfwWindow);
        engine->hiddenGLFWWindow = glfwWindow;  // Store for cleanup
#else
        // Non-Windows: use provided window handle
        void* glfwHandle = engine->windowHandle;
#endif
        
        // Create Vulkan device
        engine->device = new FirstEngine::Device::VulkanDevice();
        if (!engine->device->Initialize(glfwHandle)) {
            delete engine->device;
            engine->device = nullptr;
#ifdef _WIN32
            if (engine->hiddenGLFWWindow) {
                glfwDestroyWindow(engine->hiddenGLFWWindow);
                engine->hiddenGLFWWindow = nullptr;
            }
            glfwTerminate();
#endif
            return false;
        }
        
        // Get renderer for accessing Vulkan instance
        engine->renderer = engine->device->GetVulkanRenderer();
        if (!engine->renderer) {
            delete engine->device;
            engine->device = nullptr;
#ifdef _WIN32
            if (engine->hiddenGLFWWindow) {
                glfwDestroyWindow(engine->hiddenGLFWWindow);
                engine->hiddenGLFWWindow = nullptr;
            }
            glfwTerminate();
#endif
            return false;
        }
        
        // Initialize ShaderModuleTools with device (for creating GPU shader modules)
        auto& moduleTools = FirstEngine::Renderer::ShaderModuleTools::GetInstance();
        moduleTools.Initialize(engine->device);
        
        // Create FrameGraph
        engine->frameGraph = new FirstEngine::Renderer::FrameGraph(engine->device);
        
        // Create render pipeline
        engine->renderPipeline = new FirstEngine::Renderer::DeferredRenderPipeline(engine->device);
        
        engine->initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing engine: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unknown error initializing engine" << std::endl;
        return false;
    }
}

void EditorAPI_ShutdownEngine(RenderEngine* engine) {
    if (!engine || !engine->initialized) return;
    
    if (engine->frameGraph) {
        delete engine->frameGraph;
        engine->frameGraph = nullptr;
    }
    
    if (engine->renderPipeline) {
        delete engine->renderPipeline;
        engine->renderPipeline = nullptr;
    }
    
    if (engine->device) {
        engine->device->Shutdown();
        delete engine->device;
        engine->device = nullptr;
    }
    
#ifdef _WIN32
    // Cleanup hidden GLFW window if we created it
    if (engine->hiddenGLFWWindow) {
        glfwDestroyWindow(engine->hiddenGLFWWindow);
        engine->hiddenGLFWWindow = nullptr;
        // Note: We don't call glfwTerminate() here as it might be used elsewhere
        // The hidden window is only created for editor mode
    }
#endif
    
    // Shutdown singleton managers
    FirstEngine::Renderer::ShaderModuleTools::Shutdown();
    FirstEngine::Renderer::ShaderCollectionsTools::Shutdown();
    FirstEngine::Renderer::RenderResourceManager::Shutdown();
    
    engine->renderer = nullptr;
    engine->initialized = false;
}

// Viewport management
RenderViewport* EditorAPI_CreateViewport(RenderEngine* engine, void* parentWindowHandle, int x, int y, int width, int height) {
    if (!engine || !engine->initialized) return nullptr;
    
    auto* viewport = new RenderViewport();
    viewport->engine = engine;
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
    
    // For now, use parent window handle (swapchain will render to main window)
    // TODO: Create separate surface and swapchain for child window
    // Note: We need to pass a GLFWwindow*, but we have HWND
    // For editor mode, we'll create swapchain using parent window
    // The child window will be used for embedding, but rendering goes to parent swapchain
    auto swapchainPtr = engine->device->CreateSwapchain(hwndParent, swapchainDesc);
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
    
    auto swapchainPtr = engine->device->CreateSwapchain(parentWindowHandle, swapchainDesc);
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
    }
#else
    if (viewport->swapchain && width > 0 && height > 0) {
        viewport->swapchain->Recreate();
    }
#endif
}

void EditorAPI_SetViewportActive(RenderViewport* viewport, bool active) {
    if (!viewport) return;
    viewport->active = active;
}

// Frame rendering
void EditorAPI_BeginFrame(RenderEngine* engine) {
    if (!engine || !engine->initialized) return;
    
    // Build FrameGraph for this frame
    if (engine->frameGraph && engine->renderPipeline) {
        engine->frameGraph->Clear();
        engine->renderPipeline->BuildFrameGraph(*engine->frameGraph, engine->renderConfig);
    }
}

void EditorAPI_EndFrame(RenderEngine* engine) {
    if (!engine || !engine->initialized) return;
    // Frame end logic - could flush commands, wait for GPU, etc.
}

void EditorAPI_RenderViewport(RenderViewport* viewport) {
    if (!viewport || !viewport->active || !viewport->ready) return;
    if (!viewport->engine || !viewport->engine->initialized) return;
    
    // Render logic for viewport
    // This would typically involve:
    // 1. Acquire swapchain image
    // 2. Record command buffer with render commands
    // 3. Submit command buffer
    // 4. Present swapchain image
    // 
    // For now, this is a placeholder - actual rendering is handled by the main render loop
    // The viewport window is created and embedded, but rendering goes through the main swapchain
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
void EditorAPI_LoadScene(RenderEngine* engine, const char* scenePath) {
    if (!engine || !scenePath) return;
    // Load scene logic
}

void EditorAPI_UnloadScene(RenderEngine* engine) {
    if (!engine) return;
    // Unload scene logic
}

// Resource management
void EditorAPI_LoadResource(RenderEngine* engine, const char* resourcePath, int resourceType) {
    if (!engine || !resourcePath) return;
    // Load resource logic
}

void EditorAPI_UnloadResource(RenderEngine* engine, const char* resourcePath) {
    if (!engine || !resourcePath) return;
    // Unload resource logic
}

// Configuration
void EditorAPI_SetRenderConfig(RenderEngine* engine, int width, int height, float renderScale) {
    if (!engine) return;
    engine->renderConfig.SetResolution(width, height);
    // Set render scale if supported
}

} // extern "C"
