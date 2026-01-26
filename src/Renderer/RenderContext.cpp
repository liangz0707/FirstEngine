#include "FirstEngine/Renderer/RenderContext.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Resources/DefaultTextures.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Device/VulkanRenderer.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Renderer {

        RenderContext::RenderContext() = default;

        RenderContext::~RenderContext() {
            ShutdownEngine();
        }

        bool RenderContext::CreateSyncObjects(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            if (!m_InFlightFence) {
                m_InFlightFence = device->CreateFence(true); // Start signaled
                if (!m_InFlightFence) {
                    std::cerr << "Failed to create in-flight fence in RenderContext" << std::endl;
                    return false;
                }
            }

            if (!m_ImageAvailableSemaphore) {
                m_ImageAvailableSemaphore = device->CreateSemaphoreHandle();
                if (!m_ImageAvailableSemaphore) {
                    std::cerr << "Failed to create image available semaphore in RenderContext" << std::endl;
                    DestroySyncObjects(device);
                    return false;
                }
            }

            if (!m_RenderFinishedSemaphore) {
                m_RenderFinishedSemaphore = device->CreateSemaphoreHandle();
                if (!m_RenderFinishedSemaphore) {
                    std::cerr << "Failed to create render finished semaphore in RenderContext" << std::endl;
                    DestroySyncObjects(device);
                    return false;
                }
            }

            return true;
        }

        void RenderContext::DestroySyncObjects(RHI::IDevice* device) {
            if (!device) {
                return;
            }

            if (m_InFlightFence) {
                device->DestroyFence(m_InFlightFence);
                m_InFlightFence = nullptr;
            }

            if (m_RenderFinishedSemaphore) {
                device->DestroySemaphore(m_RenderFinishedSemaphore);
                m_RenderFinishedSemaphore = nullptr;
            }

            if (m_ImageAvailableSemaphore) {
                device->DestroySemaphore(m_ImageAvailableSemaphore);
                m_ImageAvailableSemaphore = nullptr;
            }
        }

        bool RenderContext::BeginFrame() {
            
            if (!m_RenderPipeline || !m_FrameGraph) {
                std::cerr << "RenderContext::BeginFrame: Engine not initialized" << std::endl;
                return false;
            }

            // Don't immediately release resources - they may still be in use by the previous frame
            // Instead, mark nodes that are being removed for destruction
            // Resources will be destroyed in ProcessResources after the frame is submitted
            m_FrameGraph->MarkRemovedNodesForDestruction();

            m_FrameGraph->Clear();


            if (!m_RenderPipeline->BuildFrameGraph(*m_FrameGraph, m_RenderConfig)) {
                std::cerr << "RenderContext::BeginFrame: Failed to build FrameGraph" << std::endl;
                return false;
            }

            if (!m_RenderPipeline->BuildExecutionPlan(*m_FrameGraph, m_ExecutionPlan)) {
                std::cerr << "RenderContext::BeginFrame: Failed to build execution plan" << std::endl;
                return false;
            }

            if (!m_RenderPipeline->Compile(*m_FrameGraph, m_ExecutionPlan)) {
                std::cerr << "RenderContext::BeginFrame: Failed to compile FrameGraph" << std::endl;
                return false;
            }

            return true;
        }

        bool RenderContext::ExecuteFrameGraph() {
            if (!m_FrameGraph || !m_Scene) {
                std::cerr << "RenderContext::ExecuteFrameGraph: Engine not initialized" << std::endl;
                return false;
            }

            m_RenderCommands.Clear();

            m_RenderCommands = m_FrameGraph->Execute(m_ExecutionPlan, m_Scene, m_RenderConfig);

            // Debug: Check if commands were generated
            if (m_RenderCommands.IsEmpty()) {
                std::cerr << "Warning: RenderContext::ExecuteFrameGraph: FrameGraph::Execute returned empty command list!" << std::endl;
                std::cerr << "  Execution plan has " << m_ExecutionPlan.GetExecutionOrder().size() << " nodes" << std::endl;
                std::cerr << "  FrameGraph has " << m_FrameGraph->GetNodeCount() << " nodes" << std::endl;
            } else {
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
                std::cout << "[EditorAPI] RenderContext::ExecuteFrameGraph: Generated " 
                          << m_RenderCommands.GetCommands().size() << " commands" << std::endl;
#endif
            }

            return true;
        }

        void RenderContext::ProcessResources(RHI::IDevice* device, uint32_t maxResourcesPerFrame) {
            
            RHI::IDevice* targetDevice = device ? device : m_Device;
            if (!targetDevice) {
                return;
            }
            
            // IMPORTANT: Process resources in the correct order
            // 1. First, process create/update operations (these don't require waiting)
            // 2. Then, wait for GPU to finish before destroying resources
            // 3. Finally, process destroy operations
            
            // Process resources managed by RenderResourceManager (create/update operations)
            RenderResourceManager::GetInstance().ProcessScheduledResources(targetDevice, maxResourcesPerFrame);
            
            // Process create/update operations for FrameGraph node resources
            if (m_FrameGraph) {
                uint32_t nodeCount = m_FrameGraph->GetNodeCount();
                
                for (uint32_t i = 0; i < nodeCount; ++i) {
                    auto* node = m_FrameGraph->GetNode(i);
                    if (!node) continue;
                    
                    // Process framebuffer resource (create/update operations only)
                    auto* framebufferResource = node->GetFramebufferResource();
                    if (framebufferResource && framebufferResource->IsScheduled()) {
                        ScheduleState scheduleState = framebufferResource->GetScheduleState();
                        // Process create/update operations (not destroy)
                        if (scheduleState == ScheduleState::ScheduledCreate || scheduleState == ScheduleState::ScheduledUpdate) {
                            framebufferResource->ProcessScheduled(targetDevice);
                        }
                    }
                    
                    // Process render pass resource (create/update operations only)
                    auto* renderPassResource = node->GetRenderPassResource();
                    if (renderPassResource && renderPassResource->IsScheduled()) {
                        ScheduleState scheduleState = renderPassResource->GetScheduleState();
                        // Process create/update operations (not destroy)
                        if (scheduleState == ScheduleState::ScheduledCreate || scheduleState == ScheduleState::ScheduledUpdate) {
                            renderPassResource->ProcessScheduled(targetDevice);
                        }
                    }
                }
            }
            
            // IMPORTANT: Wait for GPU to finish before destroying resources
            // This ensures that resources marked for destruction are no longer in use by command buffers
            // Resources scheduled for destruction in MarkRemovedNodesForDestruction() will be destroyed here
            // but only after the previous frame's command buffer has completed execution
            // Note: This should be called AFTER frame submission, not before
            // For now, we'll check if there are any resources scheduled for destruction before waiting
            bool hasResourcesToDestroy = false;
            if (m_FrameGraph) {
                uint32_t nodeCount = m_FrameGraph->GetNodeCount();
                for (uint32_t i = 0; i < nodeCount; ++i) {
                    auto* node = m_FrameGraph->GetNode(i);
                    if (!node) continue;
                    
                    auto* framebufferResource = node->GetFramebufferResource();
                    if (framebufferResource && framebufferResource->GetScheduleState() == ScheduleState::ScheduledDestroy) {
                        hasResourcesToDestroy = true;
                        break;
                    }
                    
                    auto* renderPassResource = node->GetRenderPassResource();
                    if (renderPassResource && renderPassResource->GetScheduleState() == ScheduleState::ScheduledDestroy) {
                        hasResourcesToDestroy = true;
                        break;
                    }
                }
            }
            
            // Only wait if there are resources to destroy
            if (hasResourcesToDestroy) {
                targetDevice->WaitIdle();
            }
            
            // Process destroy operations for FrameGraph node resources
            // Only destroy resources that are scheduled for destruction (not resources that are being reused)
            if (m_FrameGraph) {
                uint32_t nodeCount = m_FrameGraph->GetNodeCount();
                uint32_t processedCount = 0;
                
                for (uint32_t i = 0; i < nodeCount && processedCount < maxResourcesPerFrame; ++i) {
                    auto* node = m_FrameGraph->GetNode(i);
                    if (!node) continue;
                    
                    // Process framebuffer resource (only if scheduled for destruction)
                    auto* framebufferResource = node->GetFramebufferResource();
                    if (framebufferResource && framebufferResource->IsScheduled()) {
                        ScheduleState scheduleState = framebufferResource->GetScheduleState();
                        // Only process if scheduled for destruction (not for create/update)
                        if (scheduleState == ScheduleState::ScheduledDestroy) {
                            framebufferResource->ProcessScheduled(targetDevice);
                            processedCount++;
                        }
                    }
                    
                    // Process render pass resource (only if scheduled for destruction)
                    auto* renderPassResource = node->GetRenderPassResource();
                    if (renderPassResource && renderPassResource->IsScheduled()) {
                        ScheduleState scheduleState = renderPassResource->GetScheduleState();
                        // Only process if scheduled for destruction (not for create/update)
                        if (scheduleState == ScheduleState::ScheduledDestroy) {
                            renderPassResource->ProcessScheduled(targetDevice);
                            processedCount++;
                        }
                    }
                }
            }
        }

        bool RenderContext::SubmitFrame(const RenderParams& params) {
            if (!params.swapchain) {
                std::cerr << "RenderContext::SubmitFrame: Swapchain is required" << std::endl;
                return false;
            }

            // Use internally managed objects
            if (!m_Device) {
                std::cerr << "RenderContext::SubmitFrame: No device available" << std::endl;
                return false;
            }

            if (!m_InFlightFence || !m_ImageAvailableSemaphore || !m_RenderFinishedSemaphore) {
                std::cerr << "RenderContext::SubmitFrame: Missing synchronization objects" << std::endl;
                return false;
            }

            // Wait for previous frame to complete
            m_Device->WaitForFence(m_InFlightFence, UINT64_MAX);
            m_Device->ResetFence(m_InFlightFence);

            // Use internally managed command buffer
            m_CommandBuffer.reset();

            // Acquire next frame's swapchain image
            if (!params.swapchain->AcquireNextImage(m_ImageAvailableSemaphore, nullptr, m_CurrentImageIndex)) {
                // Image acquisition failed or need to recreate swapchain
                return false;
            }

            // Create command buffer
            m_CommandBuffer = m_Device->CreateCommandBuffer();
            if (!m_CommandBuffer) {
                return false;
            }

            // Begin recording
            m_CommandBuffer->Begin();

            // Transition swapchain image layout: Undefined → Color Attachment
            // Use Write access mode to transition to COLOR_ATTACHMENT_OPTIMAL
            auto* swapchainImage = params.swapchain->GetImage(m_CurrentImageIndex);
            if (swapchainImage) {
                m_CommandBuffer->TransitionImageLayout(
                    swapchainImage,
                    RHI::Format::Undefined,
                    RHI::Format::B8G8R8A8_UNORM,
                    1,
                    RHI::ImageAccessMode::Write  // Write mode for color attachment
                );
            }

            // Record render commands (using internally managed command recorder)
            if (m_RenderCommands.IsEmpty()) {
                std::cerr << "Warning: RenderContext::SubmitFrame: RenderCommandList is empty! No commands to record." << std::endl;
            } else {
#ifdef FE_EDITOR_API_VERBOSE_LOGGING
                std::cout << "[EditorAPI] RenderContext::SubmitFrame: Recording " 
                          << m_RenderCommands.GetCommands().size() << " commands" << std::endl;
#endif
                m_CommandRecorder.RecordCommands(m_CommandBuffer.get(), m_RenderCommands);
            }

            // Transition swapchain image layout: Color Attachment → Present
            // Use Read access mode - TransitionImageLayout will detect swapchain image and convert to PRESENT_SRC_KHR
            if (swapchainImage) {
                m_CommandBuffer->TransitionImageLayout(
                    swapchainImage,
                    RHI::Format::B8G8R8A8_UNORM,
                    RHI::Format::B8G8R8A8_SRGB,
                    1,
                    RHI::ImageAccessMode::Read  // Read mode will trigger PRESENT_SRC_KHR for swapchain images
                );
            }

            // End recording
            m_CommandBuffer->End();

            // Submit command buffer
            std::vector<RHI::SemaphoreHandle> waitSemaphores = {m_ImageAvailableSemaphore};
            std::vector<RHI::SemaphoreHandle> signalSemaphores = {m_RenderFinishedSemaphore};
            m_Device->SubmitCommandBuffer(m_CommandBuffer.get(), waitSemaphores, signalSemaphores, m_InFlightFence);

            // Present image
            std::vector<RHI::SemaphoreHandle> presentWaitSemaphores = {m_RenderFinishedSemaphore};
            params.swapchain->Present(m_CurrentImageIndex, presentWaitSemaphores);

            // IMPORTANT: Process resource destruction AFTER frame submission
            // This ensures resources are destroyed only after the command buffer has been submitted
            // The fence will ensure GPU completion before destruction in ProcessResources
            // Note: We process resources here instead of in OnCreateResources to ensure proper timing
            ProcessResources(nullptr, 0);

            return true;
        }

        // ========== Rendering Engine State Management (for EditorAPI) ==========

        bool RenderContext::InitializeEngine(void* windowHandle, int width, int height) {
            if (m_EngineInitialized) {
                return true;
            }

            try {
                // Initialize singleton managers
                RenderResourceManager::Initialize();
                
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
                m_HiddenGLFWWindow = glfwWindow;  // Store for cleanup
#else
                // Non-Windows: use provided window handle
                void* glfwHandle = windowHandle;
#endif
                
                // Create Vulkan device
                m_Device = new FirstEngine::Device::VulkanDevice();
                if (!m_Device->Initialize(glfwHandle)) {
                    delete m_Device;
                    m_Device = nullptr;
#ifdef _WIN32
                    if (m_HiddenGLFWWindow) {
                        glfwDestroyWindow(static_cast<GLFWwindow*>(m_HiddenGLFWWindow));
                        m_HiddenGLFWWindow = nullptr;
                    }
                    glfwTerminate();
#endif
                    return false;
                }
                
                // Initialize ShaderModuleTools with device
                auto& moduleTools = ShaderModuleTools::GetInstance();
                moduleTools.Initialize(m_Device);
                
                // Initialize DefaultTextureManager with device
                auto& defaultTextureManager = Resources::DefaultTextureManager::GetInstance();
                if (!defaultTextureManager.Initialize(m_Device)) {
                    std::cerr << "Warning: Failed to initialize DefaultTextureManager" << std::endl;
                }
                
                // Create FrameGraph
                m_FrameGraph = new FrameGraph(m_Device);
                
                // Create render pipeline
                m_RenderPipeline = new DeferredRenderPipeline(m_Device);
                
                // Create synchronization objects
                m_ImageAvailableSemaphore = m_Device->CreateSemaphoreHandle();
                m_RenderFinishedSemaphore = m_Device->CreateSemaphoreHandle();
                m_InFlightFence = m_Device->CreateFence(true); // Start signaled
                
                if (!m_ImageAvailableSemaphore || !m_RenderFinishedSemaphore || !m_InFlightFence) {
                    std::cerr << "Failed to create synchronization objects for RenderContext" << std::endl;
                    delete m_RenderPipeline;
                    m_RenderPipeline = nullptr;
                    delete m_FrameGraph;
                    m_FrameGraph = nullptr;
                    m_Device->Shutdown();
                    delete m_Device;
                    m_Device = nullptr;
#ifdef _WIN32
                    if (m_HiddenGLFWWindow) {
                        glfwDestroyWindow(static_cast<GLFWwindow*>(m_HiddenGLFWWindow));
                        m_HiddenGLFWWindow = nullptr;
                    }
                    glfwTerminate();
#endif
                    return false;
                }
                
                // Load Package resources through RenderResourceManager
                // Try to load from config file first, then fallback to default path
                auto& resourceManager = RenderResourceManager::GetInstance();
                std::string configPath = "engine.ini";
                if (!resourceManager.LoadPackageResources(configPath)) {
                    // Fallback: try to load from default Package path
                    std::cout << "RenderContext: Config file not found or invalid, trying default Package path" << std::endl;
                    std::string defaultPackagePath = "build/Package";
                    if (!resourceManager.LoadPackageResourcesFromPath(defaultPackagePath)) {
                        std::cerr << "RenderContext: Failed to load Package resources from default path" << std::endl;
                    }
                }
                
                // Create a default scene
                m_Scene = new FirstEngine::Resources::Scene("Editor Scene");
                
                // Initialize render config
                m_RenderConfig.SetResolution(width, height);
                m_WindowHandle = windowHandle;
                
                m_EngineInitialized = true;
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error initializing RenderContext engine: " << e.what() << std::endl;
                ShutdownEngine();
                return false;
            } catch (...) {
                std::cerr << "Unknown error initializing RenderContext engine" << std::endl;
                ShutdownEngine();
                return false;
            }
        }

        bool RenderContext::InitializeForWindow(void* windowHandle, int width, int height) {
            if (m_EngineInitialized) {
                return true;
            }
            // RenderApp mode: Use given windowHandle, don't create hidden GLFW window
            RenderResourceManager::Initialize();
            m_Device = new FirstEngine::Device::VulkanDevice();
            if (!m_Device->Initialize(windowHandle)) {
                delete m_Device;
                m_Device = nullptr;
                return false;
            }
            auto& moduleTools = ShaderModuleTools::GetInstance();
            moduleTools.Initialize(m_Device);
            m_RenderConfig.SetResolution(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            m_RenderPipeline = new DeferredRenderPipeline(m_Device);
            m_FrameGraph = new FrameGraph(m_Device);
            m_ImageAvailableSemaphore = m_Device->CreateSemaphoreHandle();
            m_RenderFinishedSemaphore = m_Device->CreateSemaphoreHandle();
            m_InFlightFence = m_Device->CreateFence(true);
            if (!m_ImageAvailableSemaphore || !m_RenderFinishedSemaphore || !m_InFlightFence) {
                if (m_InFlightFence) m_Device->DestroyFence(m_InFlightFence);
                if (m_RenderFinishedSemaphore) m_Device->DestroySemaphore(m_RenderFinishedSemaphore);
                if (m_ImageAvailableSemaphore) m_Device->DestroySemaphore(m_ImageAvailableSemaphore);
                delete m_RenderPipeline;
                delete m_FrameGraph;
                m_Device->Shutdown();
                delete m_Device;
                m_Device = nullptr;
                return false;
            }
            // Load Package resources through RenderResourceManager
            // Try to load from config file first, then fallback to default path
            auto& resourceManager = RenderResourceManager::GetInstance();
            std::string configPath = "engine.ini";
            if (!resourceManager.LoadPackageResources(configPath)) {
                // Fallback: try to load from default Package path
                std::cout << "RenderContext: Config file not found or invalid, trying default Package path" << std::endl;
                std::string defaultPackagePath = "build/Package";
                if (!resourceManager.LoadPackageResourcesFromPath(defaultPackagePath)) {
                    std::cerr << "RenderContext: Failed to load Package resources from default path" << std::endl;
                }
            }
            
            m_Scene = new FirstEngine::Resources::Scene("Example Scene");
            m_WindowHandle = windowHandle;
            m_EngineInitialized = true;
            return true;
        }

        void RenderContext::ShutdownEngine() {
            if (!m_EngineInitialized) {
                return;
            }

            // Wait for GPU to finish
            if (m_Device) {
                m_Device->WaitIdle();
            }
            
            // Cleanup DefaultTextureManager
            auto& defaultTextureManager = Resources::DefaultTextureManager::GetInstance();
            defaultTextureManager.Cleanup();

            // Release command buffer
            m_CommandBuffer.reset();

            // Destroy synchronization objects
            if (m_Device) {
                if (m_InFlightFence) {
                    m_Device->DestroyFence(m_InFlightFence);
                    m_InFlightFence = nullptr;
                }
                if (m_RenderFinishedSemaphore) {
                    m_Device->DestroySemaphore(m_RenderFinishedSemaphore);
                    m_RenderFinishedSemaphore = nullptr;
                }
                if (m_ImageAvailableSemaphore) {
                    m_Device->DestroySemaphore(m_ImageAvailableSemaphore);
                    m_ImageAvailableSemaphore = nullptr;
                }
            }

            // Cleanup scene, FrameGraph, Pipeline, Device (RenderContext owns and deletes)
            if (m_Scene) {
                delete m_Scene;
                m_Scene = nullptr;
            }
            if (m_FrameGraph) {
                delete m_FrameGraph;
                m_FrameGraph = nullptr;
            }
            if (m_RenderPipeline) {
                delete m_RenderPipeline;
                m_RenderPipeline = nullptr;
            }
            if (m_Device) {
                m_Device->Shutdown();
                delete m_Device;
                m_Device = nullptr;
            }

#ifdef _WIN32
            // Editor mode only: Destroy hidden GLFW window; ResourceManager only Shutdown in Editor path
            if (m_HiddenGLFWWindow) {
                glfwDestroyWindow(static_cast<GLFWwindow*>(m_HiddenGLFWWindow));
                m_HiddenGLFWWindow = nullptr;
                FirstEngine::Resources::ResourceManager::Shutdown();
            }
#endif

            // Singleton Shutdown (required for both modes)
            ShaderModuleTools::Shutdown();
            ShaderCollectionsTools::Shutdown();
            RenderResourceManager::Shutdown();

            m_EngineInitialized = false;
        }

        void RenderContext::SetRenderConfig(int width, int height, float renderScale) {
            m_RenderConfig.SetResolution(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            // Note: RenderConfig doesn't have SetRenderScale, but we can add it if needed
            // For now, just set resolution
            (void)renderScale;  // Unused for now
        }

        bool RenderContext::LoadScene(const std::string& scenePath) {
            if (!m_Scene) {
                m_Scene = new FirstEngine::Resources::Scene("Editor Scene");
            }

            if (fs::exists(scenePath)) {
                if (FirstEngine::Resources::SceneLoader::LoadFromFile(scenePath, *m_Scene)) {
                    std::cout << "RenderContext: Loaded scene from: " << scenePath << std::endl;
                    return true;
                }
            } else {
                std::cout << "RenderContext: Scene file not found: " << scenePath << std::endl;
            }
            return false;
        }

        void RenderContext::UnloadScene() {
            if (m_Scene) {
                delete m_Scene;
                m_Scene = new FirstEngine::Resources::Scene("Editor Scene");
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
