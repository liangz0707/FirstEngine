#include "FirstEngine/Core/Application.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Core/CommandLine.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
// SceneRenderer is now owned by Passes, no need to include here
#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/CommandRecorder.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/Component.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/RHI/ISwapchain.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#ifdef _WIN32
// RenderDoc integration
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#undef CreateSemaphore  // Undefine Windows API macro to avoid conflict with our method

// RenderDoc API structure (simplified - only the functions we need)
// Note: This structure must match RenderDoc's actual API structure layout
// The function pointers must be in the correct order
typedef struct RENDERDOC_API_1_4_2 {
    void (*StartFrameCapture)(void* device, void* wndHandle);
    void (*EndFrameCapture)(void* device, void* wndHandle);
    void (*SetCaptureOptionU32)(uint32_t opt, uint32_t val);
    void (*SetCaptureOptionF32)(uint32_t opt, float val);
    uint32_t (*GetCapture)(uint32_t idx, char* logfile, uint32_t* pathlength, uint64_t* timestamp);
    void (*TriggerCapture)(void);
    uint32_t (*IsTargetControlConnected)(void);
    uint32_t (*LaunchReplayUI)(uint32_t connectRemote, const char* cmdline);
    void (*SetActiveWindow)(void* device, void* wndHandle);
    uint32_t (*IsFrameCapturing)(void);
    void (*SetCaptureFileTemplate)(const char* pathtemplate);
    const char* (*GetCaptureFileTemplate)(void);
    uint32_t (*GetNumCaptures)(void);
    void (*ShowReplayUI)(void);
    void (*SetLogFile)(const char* logfile);
    void (*LogMessage)(uint32_t severity, const char* message);
} RENDERDOC_API_1_4_2;

typedef int (*pRENDERDOC_GetAPI)(int, void*);
#define eRENDERDOC_API_Version_1_4_2 10402
#endif

class RenderApp : public FirstEngine::Core::Application {
public:
    RenderApp(int width, int height, const std::string& title, bool headless)
        : Application(width, height, title, headless),
          m_Device(nullptr),
          m_FrameGraph(nullptr),
          m_CommandBuffer(nullptr),
          m_Scene(nullptr),
          m_RenderConfig(),
          m_CurrentImageIndex(0),
          m_LastTime(std::chrono::high_resolution_clock::now()) {
        // Initialize render config with window dimensions
        m_RenderConfig.SetResolution(width, height);
    }

    ~RenderApp() override {
        // Wait for GPU to finish before destroying resources
        if (m_Device) {
            m_Device->WaitIdle();
        }

        // Release command buffer (GPU has finished, safe to destroy)
        m_CommandBuffer.reset();

        // Destroy synchronization objects
        if (m_Device) {
            if (m_InFlightFence) {
                m_Device->DestroyFence(m_InFlightFence);
            }
            if (m_RenderFinishedSemaphore) {
                m_Device->DestroySemaphore(m_RenderFinishedSemaphore);
            }
            if (m_ImageAvailableSemaphore) {
                m_Device->DestroySemaphore(m_ImageAvailableSemaphore);
            }
        }

        m_Swapchain.reset();

        // SceneRenderer is now owned by Passes, no need to delete here
        if (m_Scene) {
            delete m_Scene;
        }
        if (m_FrameGraph) {
            delete m_FrameGraph;
        }
        if (m_RenderPipeline) {
            delete m_RenderPipeline;
        }
        if (m_Device) {
            m_Device->Shutdown();
            delete m_Device;
        }

        // Shutdown RenderResourceManager singleton
        FirstEngine::Renderer::RenderResourceManager::Shutdown();
        
        // Shutdown ShaderModuleTools singleton
        FirstEngine::Renderer::ShaderModuleTools::Shutdown();
    }

    bool Initialize() override {
        // Initialize RenderDoc BEFORE creating Vulkan instance
        // This is critical - RenderDoc needs to intercept Vulkan calls from the start
        InitializeRenderDoc();

        // Initialize RenderResourceManager singleton
        FirstEngine::Renderer::RenderResourceManager::Initialize();

        // Create device (this will create Vulkan instance)
        m_Device = new FirstEngine::Device::VulkanDevice();
        void* windowHandle = GetWindow() ? GetWindow()->GetHandle() : nullptr;
        if (!windowHandle) {
            std::cerr << "Error: No window handle available!" << std::endl;
            return false;
        }
        
        if (!m_Device->Initialize(windowHandle)) {
            std::cerr << "Failed to initialize Vulkan device!" << std::endl;
            return false;
        }

        // Create render pipeline based on RenderConfig
        const auto& pipelineConfig = m_RenderConfig.GetPipelineConfig();
        
        // Create pipeline instance based on type
        switch (pipelineConfig.type) {
            case FirstEngine::Renderer::RenderPipelineType::Deferred: {
                m_RenderPipeline = new FirstEngine::Renderer::DeferredRenderPipeline(m_Device);
                break;
            }
            case FirstEngine::Renderer::RenderPipelineType::Forward: {
                // TODO: Implement ForwardRenderPipeline
                std::cerr << "Forward rendering pipeline not yet implemented, falling back to deferred." << std::endl;
                m_RenderPipeline = new FirstEngine::Renderer::DeferredRenderPipeline(m_Device);
                break;
            }
            default: {
                std::cerr << "Unknown pipeline type, using default deferred rendering." << std::endl;
                m_RenderPipeline = new FirstEngine::Renderer::DeferredRenderPipeline(m_Device);
                break;
            }
        }

        // Create FrameGraph
        m_FrameGraph = new FirstEngine::Renderer::FrameGraph(m_Device);
        
        // Note: FrameGraph will be rebuilt every frame in OnPrepareFrameGraph()
        // This allows per-frame adjustments to the rendering pipeline configuration
        // Initial build is not needed here as it will be done in the first OnPrepareFrameGraph() call

        // Create swapchain
        if (GetWindow()) {
            // Use RenderConfig for resolution
            const auto& resolution = m_RenderConfig.GetResolution();
            FirstEngine::RHI::SwapchainDescription swapchainDesc;
            swapchainDesc.width = resolution.width;
            swapchainDesc.height = resolution.height;
            m_Swapchain = m_Device->CreateSwapchain(GetWindow()->GetHandle(), swapchainDesc);
            if (!m_Swapchain) {
                std::cerr << "Failed to create swapchain!" << std::endl;
                return false;
            }
        }

        // Create synchronization objects
        m_ImageAvailableSemaphore = m_Device->CreateSemaphore();
        m_RenderFinishedSemaphore = m_Device->CreateSemaphore();
        m_InFlightFence = m_Device->CreateFence(true); // Start signaled

        if (!m_ImageAvailableSemaphore || !m_RenderFinishedSemaphore || !m_InFlightFence) {
            std::cerr << "Failed to create synchronization objects!" << std::endl;
            return false;
        }

        // Initialize ResourceManager
        FirstEngine::Resources::ResourceManager::Initialize();
        FirstEngine::Resources::ResourceManager& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();

        // Initialize ShaderModuleTools (loads all shaders from Package/Shaders)
        auto& shaderTools = FirstEngine::Renderer::ShaderModuleTools::GetInstance();
        if (!shaderTools.Initialize("build/Package/Shaders", m_Device)) {
            std::cerr << "Warning: Failed to initialize ShaderModuleTools, shaders may not be available." << std::endl;
        } else {
            // Set device for lazy shader module creation
            shaderTools.SetDevice(m_Device);
            std::cout << "ShaderModuleTools initialized successfully." << std::endl;
        }

        // Helper function to resolve path (try multiple locations)
        auto resolvePath = [](const std::string& relativePath) -> std::string {
            // Normalize path separators
            std::string normalizedPath = relativePath;
            std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
            
            // Try current working directory first
            if (fs::exists(normalizedPath)) {
                return fs::absolute(normalizedPath).string();
            }
            
            // Try relative to executable directory (Windows)
#ifdef _WIN32
            char exePath[MAX_PATH];
            DWORD pathLen = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            if (pathLen > 0 && pathLen < MAX_PATH) {
                // Remove executable name, keep directory
                std::string exeDir;
                for (int i = pathLen - 1; i >= 0; i--) {
                    if (exePath[i] == '\\' || exePath[i] == '/') {
                        exePath[i + 1] = '\0';
                        exeDir = exePath;
                        // Normalize to forward slashes for consistency
                        std::replace(exeDir.begin(), exeDir.end(), '\\', '/');
                        break;
                    }
                }
                
                if (!exeDir.empty()) {
                    // Try in executable directory
                    std::string fullPath = exeDir + normalizedPath;
                    if (fs::exists(fullPath)) {
                        return fs::absolute(fullPath).string();
                    }
                    // Try going up one directory (if exe is in build/Debug or build/Release)
                    std::string parentPath = exeDir + "../" + normalizedPath;
                    if (fs::exists(parentPath)) {
                        return fs::absolute(parentPath).string();
                    }
                    // Try going up two directories (if exe is in build/Debug/FirstEngine or similar)
                    std::string grandParentPath = exeDir + "../../" + normalizedPath;
                    if (fs::exists(grandParentPath)) {
                        return fs::absolute(grandParentPath).string();
                    }
                    // Try going up three directories (if exe is deep in build tree)
                    std::string greatGrandParentPath = exeDir + "../../../" + normalizedPath;
                    if (fs::exists(greatGrandParentPath)) {
                        return fs::absolute(greatGrandParentPath).string();
                    }
                }
            }
#endif
            // Return original path if not found (caller will handle error)
            return normalizedPath;
        };

        // Add Package directory as search path
        std::string packageBase = resolvePath("build/Package");
        if (fs::exists(packageBase)) {
            resourceManager.AddSearchPath(packageBase);
            resourceManager.AddSearchPath(packageBase + "/Models");
            resourceManager.AddSearchPath(packageBase + "/Materials");
            resourceManager.AddSearchPath(packageBase + "/Textures");
            resourceManager.AddSearchPath(packageBase + "/Shaders");
            resourceManager.AddSearchPath(packageBase + "/Scenes");
        } else {
            // Fallback to relative paths
            resourceManager.AddSearchPath("build/Package");
            resourceManager.AddSearchPath("build/Package/Models");
            resourceManager.AddSearchPath("build/Package/Materials");
            resourceManager.AddSearchPath("build/Package/Textures");
            resourceManager.AddSearchPath("build/Package/Shaders");
            resourceManager.AddSearchPath("build/Package/Scenes");
        }

        // Load resource manifest
        std::string manifestPath = resolvePath("build/Package/resource_manifest.json");
        if (fs::exists(manifestPath)) {
            if (resourceManager.GetIDManager().LoadManifest(manifestPath)) {
                std::cout << "Loaded resource manifest from: " << manifestPath << std::endl;
            } else {
                std::cerr << "Failed to load resource manifest!" << std::endl;
            }
        } else {
            std::cout << "Resource manifest not found: " << manifestPath << std::endl;
            std::cout << "Will register resources on demand." << std::endl;
        }

        // Create scene
        m_Scene = new FirstEngine::Resources::Scene("Example Scene");

        // Load scene from file
        std::string scenePath = resolvePath("build/Package/Scenes/example_scene.json");
        if (fs::exists(scenePath)) {
            if (FirstEngine::Resources::SceneLoader::LoadFromFile(scenePath, *m_Scene)) {
                std::cout << "Loaded scene from: " << scenePath << std::endl;
            } else {
                std::cerr << "Failed to load scene from: " << scenePath << std::endl;
            }
        } else {
            std::cerr << "Scene file not found: " << scenePath << std::endl;
            std::cerr << "Current working directory: " << fs::current_path().string() << std::endl;
        }

        // SceneRenderer is now owned by each Pass (IRenderPass)
        // No need to create a global SceneRenderer anymore

        std::cout << "RenderApp initialized successfully!" << std::endl;
        return true;
    }

protected:
    void OnUpdate(float deltaTime) override {
        // Update logic here
    }

    void OnPrepareFrameGraph() override {
        if (!m_FrameGraph || !m_RenderPipeline) {
            return;
        }

        // Rebuild FrameGraph every frame to allow dynamic configuration changes
        // This enables per-frame adjustments to the rendering pipeline
        // 
        // Note: Resource release is safe here because:
        // - OnSubmit() (previous frame) already waited for GPU to complete (WaitForFence)
        // - This is called after OnSubmit(), so GPU is guaranteed to be done with old resources
        // - New resources will be allocated after this, and used in the next OnSubmit()
        
        // Release old resources (from previous frame)
        m_FrameGraph->ReleaseResources();
        
        // Clear the graph structure
        m_FrameGraph->Clear();
        
        // Rebuild FrameGraph structure from pipeline (using current RenderConfig)
        // This allows the pipeline to be reconfigured per frame based on RenderConfig changes
        if (!m_RenderPipeline->BuildFrameGraph(*m_FrameGraph, m_RenderConfig)) {
            std::cerr << "Failed to rebuild FrameGraph!" << std::endl;
            return;
        }

        // Build FrameGraph execution plan (Device/CommandBuffer independent)
        // This determines the rendering pipeline structure and what resources are needed
        // This happens before OnRender(), so we can use this information to decide
        // which scene resources need to be collected
        if (!m_RenderPipeline->BuildExecutionPlan(*m_FrameGraph, m_FrameGraphPlan)) {
            std::cerr << "Failed to build execution plan!" << std::endl;
            return;
        }

        // Compile FrameGraph based on the execution plan (allocate new resources)
        // Note: Resource allocation happens here, after the plan structure is built
        // New resources will be used in the current frame's rendering
        if (!m_RenderPipeline->Compile(*m_FrameGraph, m_FrameGraphPlan)) {
            std::cerr << "Failed to compile FrameGraph!" << std::endl;
        }
    }

    void OnCreateResources() override {
        if (!m_Device) {
            return;
        }

        // ===== Resource Creation Phase (IRenderResource Processing) =====
        // Process scheduled resources (create/update/destroy)
        // Only processes IRenderResource* directly, no Entity/Component references
        // Supports frame-by-frame processing for performance (maxResourcesPerFrame = 0 means unlimited)
        // In production, you might want to limit this to avoid frame spikes
        uint32_t maxResourcesPerFrame = 0; // 0 = unlimited, set to a number (e.g., 10) to limit per frame
        FirstEngine::Renderer::RenderResourceManager::GetInstance().ProcessScheduledResources(m_Device, maxResourcesPerFrame);
        // ===== End Resource Creation Phase =====
    }

    void OnRender() override {
        if (!m_Scene || !m_FrameGraph) {
            return;
        }

        // Clear command lists from previous frame
        m_FrameGraphRenderCommands.Clear();

        // Update render config with current window dimensions
        if (GetWindow()) {
            m_RenderConfig.UpdateResolutionFromWindow(
                GetWindow()->GetWidth(),
                GetWindow()->GetHeight()
            );
        }

        // Execute FrameGraph to get render command list (data structure, no CommandBuffer dependency)
        // FrameGraph::Execute will:
        //   1. For each Pass with SceneRenderer, call Render() to generate SceneRenderCommands
        //   2. Pass SceneRenderCommands to OnDraw() callback
        // Scene and RenderConfig are passed to Execute, which will forward them to Passes with SceneRenderer
        m_FrameGraphRenderCommands = m_FrameGraph->Execute(m_FrameGraphPlan, m_Scene, m_RenderConfig);
    }

    void OnSubmit() override {
        if (!m_Device || !m_FrameGraph || !m_Swapchain) {
            return;
        }

        // Wait for previous frame to complete before starting new frame
        // This ensures command buffer and semaphores from previous frame are no longer in use
        m_Device->WaitForFence(m_InFlightFence, UINT64_MAX);
        m_Device->ResetFence(m_InFlightFence);

        // Now that fence is signaled, it's safe to release the previous frame's command buffer
        // The command buffer from previous frame is no longer in use by GPU
        m_CommandBuffer.reset();


        // Begin RenderDoc frame capture
        // Note: For Vulkan, RenderDoc works as an implicit layer and automatically captures frames
        // Manual capture calls are disabled to avoid access violations
        // RenderDoc will automatically capture when launched from RenderDoc UI or injected into process
        // BeginRenderDocFrame();

        // Acquire next swapchain image
        // m_ImageAvailableSemaphore will be signaled when image is available
        // After waiting for fence, the semaphore from previous frame should be unsignaled
        if (!m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore, nullptr, m_CurrentImageIndex)) {
            // Image acquisition failed or swapchain needs recreation
            return;
        }

        // Create command buffer for this frame
        m_CommandBuffer = m_Device->CreateCommandBuffer();
        if (!m_CommandBuffer) {
            return;
        }

        // Begin recording
        m_CommandBuffer->Begin();

        // Transition swapchain image from undefined to color attachment layout
        auto* swapchainImage = m_Swapchain->GetImage(m_CurrentImageIndex);
        if (swapchainImage) {
            m_CommandBuffer->TransitionImageLayout(
                swapchainImage,
                FirstEngine::RHI::Format::Undefined, // Old layout (undefined for first use)
                FirstEngine::RHI::Format::B8G8R8A8_UNORM, // New layout (color attachment)
                1 // mipLevels
            );
        }

        // Record render commands to command buffer
        // FrameGraph commands were generated in OnRender() as data structures
        // Scene commands have been merged into FrameGraph passes (geometry/forward)
        // Now we convert these data structures to actual GPU commands
        // This provides complete logical isolation between data generation and command recording
        
        // Record FrameGraph render commands (includes merged scene commands)
        // Scene commands are now part of FrameGraph passes, so we only need to record FrameGraph commands
        if (!m_FrameGraphRenderCommands.IsEmpty()) {
            m_CommandRecorder.RecordCommands(m_CommandBuffer.get(), m_FrameGraphRenderCommands);
        }

        // Transition swapchain image from color attachment to present layout
        // This is a direct CommandBuffer operation (not part of scene/FrameGraph data)
        // This must be done before ending the command buffer
        if (swapchainImage) {
            m_CommandBuffer->TransitionImageLayout(
                swapchainImage,
                FirstEngine::RHI::Format::B8G8R8A8_UNORM, // Old layout (color attachment)
                FirstEngine::RHI::Format::B8G8R8A8_SRGB, // New layout (present src - using SRGB format as indicator)
                1 // mipLevels
            );
        }

        // End recording
        m_CommandBuffer->End();

        // Submit command buffer
        // Wait for image acquisition, signal render finished
        std::vector<FirstEngine::RHI::SemaphoreHandle> waitSemaphores = {m_ImageAvailableSemaphore};
        std::vector<FirstEngine::RHI::SemaphoreHandle> signalSemaphores = {m_RenderFinishedSemaphore};
        m_Device->SubmitCommandBuffer(m_CommandBuffer.get(), waitSemaphores, signalSemaphores, m_InFlightFence);

        // Present - wait for render finished
        std::vector<FirstEngine::RHI::SemaphoreHandle> presentWaitSemaphores = {m_RenderFinishedSemaphore};
        m_Swapchain->Present(m_CurrentImageIndex, presentWaitSemaphores);

        // End RenderDoc frame capture
        // Note: For Vulkan, RenderDoc works as an implicit layer and automatically captures frames
        // Manual capture calls are disabled to avoid access violations
        // RenderDoc will automatically capture when launched from RenderDoc UI or injected into process
        // EndRenderDocFrame();

        // Note: Command buffer is stored as member variable and will be released
        // at the start of next frame after waiting for fence, ensuring GPU has finished using it
        // Semaphores are reused, not destroyed here
        // Render queue is cleared at the start of OnRender() for next frame
    }

    void OnResize(int width, int height) override {
        // Ignore resize events with invalid dimensions (window may be minimized)
        if (width <= 0 || height <= 0) {
            return;
        }

        // Update render config with new resolution
        m_RenderConfig.UpdateResolutionFromWindow(width, height);

        if (m_FrameGraph && m_Device) {
            // Wait for GPU to finish before recreating swapchain
            m_Device->WaitIdle();

            // Recreate swapchain with new dimensions using Recreate() method
            // This properly handles old swapchain destruction
            if (m_Swapchain) {
                try {
                    if (!m_Swapchain->Recreate()) {
                        std::cerr << "Failed to recreate swapchain on resize!" << std::endl;
                        return;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error recreating swapchain: " << e.what() << std::endl;
                    return;
                }
            } else if (GetWindow()) {
                // If swapchain doesn't exist yet, create it
                try {
                    FirstEngine::RHI::SwapchainDescription swapchainDesc;
                    swapchainDesc.width = width;
                    swapchainDesc.height = height;
                    m_Swapchain = m_Device->CreateSwapchain(GetWindow()->GetHandle(), swapchainDesc);
                    
                    if (!m_Swapchain) {
                        std::cerr << "Failed to create swapchain on resize!" << std::endl;
                        return;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error creating swapchain: " << e.what() << std::endl;
                    return;
                }
            }

            // Rebuild FrameGraph with new dimensions from RenderConfig
            if (m_RenderPipeline && m_FrameGraph) {
                m_FrameGraph->Clear();
                if (!m_RenderPipeline->BuildFrameGraph(*m_FrameGraph, m_RenderConfig)) {
                    std::cerr << "Failed to rebuild FrameGraph on resize!" << std::endl;
                    return;
                }
            }
        }
    }

private:
    void InitializeRenderDoc() {
#ifdef _WIN32
        // Try to load renderdoc.dll from multiple locations
        HMODULE mod = nullptr;
        
        // First, try to get already loaded module (if injected by RenderDoc)
        mod = GetModuleHandleA("renderdoc.dll");
        
        // If not found, try to load from common RenderDoc installation paths
        if (!mod) {
            const char* renderdocPaths[] = {
                "C:\\Program Files\\RenderDoc\\renderdoc.dll",
                "C:\\Program Files (x86)\\RenderDoc\\renderdoc.dll",
                nullptr
            };
            
            for (int i = 0; renderdocPaths[i] != nullptr; i++) {
                mod = LoadLibraryA(renderdocPaths[i]);
                if (mod) {
                    std::cout << "Loaded RenderDoc from: " << renderdocPaths[i] << std::endl;
                    break;
                }
            }
        }
        
        if (mod) {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            if (RENDERDOC_GetAPI) {
                RENDERDOC_API_1_4_2* rdoc_api = nullptr;
                int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_2, (void**)&rdoc_api);
                if (ret == 1 && rdoc_api) {
                    // Verify the API structure is valid by checking function pointers
                    if (rdoc_api->StartFrameCapture && rdoc_api->EndFrameCapture) {
                        m_RenderDocAPI = rdoc_api;
                        std::cout << "RenderDoc API initialized successfully!" << std::endl;
                        
                        // Set capture options (optional)
                        if (rdoc_api->SetCaptureOptionU32) {
                            rdoc_api->SetCaptureOptionU32(1, 1); // Allow VSync
                            rdoc_api->SetCaptureOptionU32(2, 0); // Allow fullscreen
                        }
                        
                        // Check if RenderDoc is connected
                        if (rdoc_api->IsTargetControlConnected && rdoc_api->IsTargetControlConnected()) {
                            std::cout << "RenderDoc target control is connected!" << std::endl;
                        } else {
                            std::cout << "RenderDoc target control is NOT connected. Make sure RenderDoc UI is running." << std::endl;
                        }
                    } else {
                        std::cout << "RenderDoc API structure is invalid (missing function pointers)." << std::endl;
                    }
                } else {
                    std::cout << "Failed to get RenderDoc API. Return code: " << ret << std::endl;
                }
            } else {
                std::cout << "Failed to get RENDERDOC_GetAPI function pointer." << std::endl;
            }
        } else {
            std::cout << "RenderDoc DLL not found. RenderDoc capture will not be available." << std::endl;
            std::cout << "To use RenderDoc:" << std::endl;
            std::cout << "  1. Install RenderDoc from https://renderdoc.org/" << std::endl;
            std::cout << "  2. Launch this application from RenderDoc UI (Launch Application)" << std::endl;
            std::cout << "  3. Or inject RenderDoc into this process (Inject into Process)" << std::endl;
        }
        // If RenderDoc is not available, that's okay - program will run normally
#endif
    }

    void BeginRenderDocFrame() {
#ifdef _WIN32
        if (m_RenderDocAPI) {
            RENDERDOC_API_1_4_2* api = (RENDERDOC_API_1_4_2*)m_RenderDocAPI;
            // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
            // This is the safest approach and avoids access violation errors
            if (api && api->StartFrameCapture) {
                try {
                    api->StartFrameCapture(nullptr, nullptr);
                } catch (...) {
                    // Silently ignore errors - RenderDoc might not be fully initialized yet
                }
            }
        }
#endif
    }

    void EndRenderDocFrame() {
#ifdef _WIN32
        if (m_RenderDocAPI) {
            RENDERDOC_API_1_4_2* api = (RENDERDOC_API_1_4_2*)m_RenderDocAPI;
            // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
            if (api && api->EndFrameCapture) {
                try {
                    api->EndFrameCapture(nullptr, nullptr);
                } catch (...) {
                    // Silently ignore errors - RenderDoc might not be fully initialized yet
                }
            }
        }
#endif
    }

    FirstEngine::Device::VulkanDevice* m_Device;
    FirstEngine::Renderer::IRenderPipeline* m_RenderPipeline = nullptr; // Render pipeline (Deferred, Forward, etc.)
    FirstEngine::Renderer::FrameGraph* m_FrameGraph;
    FirstEngine::Renderer::FrameGraphExecutionPlan m_FrameGraphPlan; // FrameGraph execution plan (Device/CommandBuffer independent)
    std::unique_ptr<FirstEngine::RHI::ISwapchain> m_Swapchain;
    FirstEngine::RHI::SemaphoreHandle m_ImageAvailableSemaphore;
    FirstEngine::RHI::SemaphoreHandle m_RenderFinishedSemaphore;
    FirstEngine::RHI::FenceHandle m_InFlightFence;
    std::unique_ptr<FirstEngine::RHI::ICommandBuffer> m_CommandBuffer; // Store command buffer to defer destruction
    FirstEngine::Resources::Scene* m_Scene;
    FirstEngine::Renderer::RenderConfig m_RenderConfig; // Global render configuration (camera, resolution, flags)
    FirstEngine::Renderer::RenderCommandList m_FrameGraphRenderCommands; // Render commands from FrameGraph (CommandBuffer independent)
    FirstEngine::Renderer::CommandRecorder m_CommandRecorder; // Converts RenderCommandList to CommandBuffer commands
    uint32_t m_CurrentImageIndex = 0; // Store current image index for OnSubmit
    std::chrono::high_resolution_clock::time_point m_LastTime;

#ifdef _WIN32
    RENDERDOC_API_1_4_2* m_RenderDocAPI = nullptr; // RenderDoc API pointer
#endif
};

int main(int argc, char* argv[]) {
    FirstEngine::Core::CommandLineParser parser;

    parser.AddArgument("width", "w", "Window width", FirstEngine::Core::ArgumentType::Integer, false, "1280");
    parser.AddArgument("height", "h", "Window height", FirstEngine::Core::ArgumentType::Integer, false, "720");
    parser.AddArgument("title", "t", "Window title", FirstEngine::Core::ArgumentType::String, false, "FirstEngine");
    parser.AddArgument("headless", "", "Run in headless mode", FirstEngine::Core::ArgumentType::Flag);
    parser.AddArgument("config", "c", "Pipeline config file path", FirstEngine::Core::ArgumentType::String);
    parser.AddArgument("help", "?", "Show help message", FirstEngine::Core::ArgumentType::Flag);

    if (!parser.Parse(argc, argv)) {
        std::cerr << "Error parsing command line: " << parser.GetError() << std::endl;
        return 1;
    }

    if (parser.GetBool("help")) {
        parser.PrintHelp("FirstEngine");
        return 0;
    }

    int width = parser.GetInt("width");
    int height = parser.GetInt("height");
    std::string title = parser.GetString("title");
    bool headless = parser.GetBool("headless");

    try {
        RenderApp app(width, height, title, headless);
        
        if (!app.Initialize()) {
            std::cerr << "Failed to initialize application!" << std::endl;
            return -1;
        }

        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
