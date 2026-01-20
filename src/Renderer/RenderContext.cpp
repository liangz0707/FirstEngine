#include "FirstEngine/Renderer/RenderContext.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Device/VulkanRenderer.h"
#include <iostream>

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

            m_FrameGraph->ReleaseResources();

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

            return true;
        }

        void RenderContext::ProcessResources(RHI::IDevice* device, uint32_t maxResourcesPerFrame) {
            
            RHI::IDevice* targetDevice = device ? device : m_Device;
            if (!targetDevice) {
                return;
            }
            RenderResourceManager::GetInstance().ProcessScheduledResources(targetDevice, maxResourcesPerFrame);
        }

        bool RenderContext::SubmitFrame(const RenderParams& params) {
            if (!params.swapchain) {
                std::cerr << "RenderContext::SubmitFrame: Swapchain is required" << std::endl;
                return false;
            }

            // 使用内部管理的对象
            if (!m_Device) {
                std::cerr << "RenderContext::SubmitFrame: No device available" << std::endl;
                return false;
            }

            if (!m_InFlightFence || !m_ImageAvailableSemaphore || !m_RenderFinishedSemaphore) {
                std::cerr << "RenderContext::SubmitFrame: Missing synchronization objects" << std::endl;
                return false;
            }

            // 等待上一帧完成
            m_Device->WaitForFence(m_InFlightFence, UINT64_MAX);
            m_Device->ResetFence(m_InFlightFence);

            // 使用内部管理的命令缓冲区
            m_CommandBuffer.reset();

            // 获取下一帧的 swapchain 图像
            if (!params.swapchain->AcquireNextImage(m_ImageAvailableSemaphore, nullptr, m_CurrentImageIndex)) {
                // 图像获取失败或需要重建 swapchain
                return false;
            }

            // 创建命令缓冲区
            m_CommandBuffer = m_Device->CreateCommandBuffer();
            if (!m_CommandBuffer) {
                return false;
            }

            // 开始记录
            m_CommandBuffer->Begin();

            // 转换 swapchain 图像布局：Undefined → Color Attachment
            auto* swapchainImage = params.swapchain->GetImage(m_CurrentImageIndex);
            if (swapchainImage) {
                m_CommandBuffer->TransitionImageLayout(
                    swapchainImage,
                    RHI::Format::Undefined,
                    RHI::Format::B8G8R8A8_UNORM,
                    1
                );
            }

            // 记录渲染命令（使用内部管理的命令记录器）
            if (!m_RenderCommands.IsEmpty()) {
                m_CommandRecorder.RecordCommands(m_CommandBuffer.get(), m_RenderCommands);
            }

            // 转换 swapchain 图像布局：Color Attachment → Present
            if (swapchainImage) {
                m_CommandBuffer->TransitionImageLayout(
                    swapchainImage,
                    RHI::Format::B8G8R8A8_UNORM,
                    RHI::Format::B8G8R8A8_SRGB,
                    1
                );
            }

            // 结束记录
            m_CommandBuffer->End();

            // 提交命令缓冲区
            std::vector<RHI::SemaphoreHandle> waitSemaphores = {m_ImageAvailableSemaphore};
            std::vector<RHI::SemaphoreHandle> signalSemaphores = {m_RenderFinishedSemaphore};
            m_Device->SubmitCommandBuffer(m_CommandBuffer.get(), waitSemaphores, signalSemaphores, m_InFlightFence);

            // 呈现图像
            std::vector<RHI::SemaphoreHandle> presentWaitSemaphores = {m_RenderFinishedSemaphore};
            params.swapchain->Present(m_CurrentImageIndex, presentWaitSemaphores);

            return true;
        }

        // ========== 渲染引擎状态管理（用于 EditorAPI） ==========

        bool RenderContext::InitializeEngine(void* windowHandle, int width, int height) {
            if (m_EngineInitialized) {
                return true;
            }

            try {
                // Initialize singleton managers
                RenderResourceManager::Initialize();
                
                // Initialize ShaderCollectionsTools with shader directory
                auto& collectionsTools = ShaderCollectionsTools::GetInstance();
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
                
                // Initialize ResourceManager
                FirstEngine::Resources::ResourceManager::Initialize();
                FirstEngine::Resources::ResourceManager& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();
                
                // Add Package directory as search path
                std::string packageBase = "build/Package";
                if (fs::exists(packageBase)) {
                    resourceManager.AddSearchPath(packageBase);
                    resourceManager.AddSearchPath(packageBase + "/Models");
                    resourceManager.AddSearchPath(packageBase + "/Materials");
                    resourceManager.AddSearchPath(packageBase + "/Textures");
                    resourceManager.AddSearchPath(packageBase + "/Shaders");
                    resourceManager.AddSearchPath(packageBase + "/Scenes");
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
            // RenderApp 模式：使用给定 windowHandle，不创建隐藏 GLFW 窗口
            RenderResourceManager::Initialize();
            auto& collectionsTools = ShaderCollectionsTools::GetInstance();
            if (!collectionsTools.Initialize("build/Package/Shaders")) {
                std::cerr << "Warning: Failed to initialize ShaderCollectionsTools." << std::endl;
            }
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
            FirstEngine::Resources::ResourceManager::Initialize();
            auto& rm = FirstEngine::Resources::ResourceManager::GetInstance();
            std::string base = "build/Package";
            rm.AddSearchPath(base);
            rm.AddSearchPath(base + "/Models");
            rm.AddSearchPath(base + "/Materials");
            rm.AddSearchPath(base + "/Textures");
            rm.AddSearchPath(base + "/Shaders");
            rm.AddSearchPath(base + "/Scenes");
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

            // Cleanup scene, FrameGraph, Pipeline, Device（RenderContext 拥有并删除）
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
            // 仅 Editor 模式：销毁隐藏 GLFW 窗口；ResourceManager 仅在 Editor 路径 Shutdown
            if (m_HiddenGLFWWindow) {
                glfwDestroyWindow(static_cast<GLFWwindow*>(m_HiddenGLFWWindow));
                m_HiddenGLFWWindow = nullptr;
                FirstEngine::Resources::ResourceManager::Shutdown();
            }
#endif

            // 单例 Shutdown（两种模式均需）
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
