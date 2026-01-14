#include "FirstEngine/Core/Application.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Core/CommandLine.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/PipelineConfig.h"
#include "FirstEngine/Renderer/PipelineBuilder.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/RHI/ISwapchain.h"
#include <iostream>
#include <chrono>
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
// RenderDoc API forward declarations
struct RENDERDOC_API_1_4_2;
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
          m_LastTime(std::chrono::high_resolution_clock::now()) {
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

        if (m_FrameGraph) {
            delete m_FrameGraph;
        }
        if (m_Device) {
            m_Device->Shutdown();
            delete m_Device;
        }
    }

    bool Initialize() override {
        // Initialize RenderDoc
        InitializeRenderDoc();

        // Create device
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

        // Load pipeline config
        std::string configPath = "configs/deferred_rendering.pipeline";
        
        // Create configs directory if it doesn't exist
        try {
            fs::create_directories("configs");
        } catch (...) {
            std::cerr << "Warning: Failed to create configs directory!" << std::endl;
        }
        
        FirstEngine::Renderer::PipelineConfig config;
        int width = GetWindow() ? GetWindow()->GetWidth() : 1280;
        int height = GetWindow() ? GetWindow()->GetHeight() : 720;
        
        if (!FirstEngine::Renderer::PipelineConfigParser::ParseFromFile(configPath, config)) {
            std::cout << "Config file not found, creating default deferred rendering config..." << std::endl;
            config = FirstEngine::Renderer::PipelineBuilder::CreateDeferredRenderingConfig(width, height);
            
            // Save config for future use
            if (!FirstEngine::Renderer::PipelineConfigParser::SaveToFile(configPath, config)) {
                std::cerr << "Warning: Failed to save config file!" << std::endl;
            }
        }

        // Build FrameGraph from config
        FirstEngine::Renderer::PipelineBuilder builder(m_Device);
        m_FrameGraph = new FirstEngine::Renderer::FrameGraph(m_Device);
        
        if (!builder.BuildFromConfig(config, *m_FrameGraph)) {
            std::cerr << "Failed to build FrameGraph from config!" << std::endl;
            return false;
        }

        // Create swapchain
        if (GetWindow()) {
            FirstEngine::RHI::SwapchainDescription swapchainDesc;
            swapchainDesc.width = GetWindow()->GetWidth();
            swapchainDesc.height = GetWindow()->GetHeight();
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

        std::cout << "RenderApp initialized successfully!" << std::endl;
        return true;
    }

protected:
    void OnUpdate(float deltaTime) override {
        // Update logic here
    }

    void OnRender() override {
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
        BeginRenderDocFrame();

        // Acquire next swapchain image
        // m_ImageAvailableSemaphore will be signaled when image is available
        // After waiting for fence, the semaphore from previous frame should be unsignaled
        uint32_t imageIndex;
        if (!m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore, nullptr, imageIndex)) {
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
        auto* swapchainImage = m_Swapchain->GetImage(imageIndex);
        if (swapchainImage) {
            m_CommandBuffer->TransitionImageLayout(
                swapchainImage,
                FirstEngine::RHI::Format::Undefined, // Old layout (undefined for first use)
                FirstEngine::RHI::Format::B8G8R8A8_UNORM, // New layout (color attachment)
                1 // mipLevels
            );
        }

        // Execute FrameGraph (this will render to the swapchain image)
        m_FrameGraph->Execute(m_CommandBuffer.get());

        // Transition swapchain image from color attachment to present layout
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
        m_Swapchain->Present(imageIndex, presentWaitSemaphores);

        // End RenderDoc frame capture
        EndRenderDocFrame();

        // Note: Command buffer is stored as member variable and will be released
        // at the start of next frame after waiting for fence, ensuring GPU has finished using it
        // Semaphores are reused, not destroyed here
    }

    void OnResize(int width, int height) override {
        if (m_FrameGraph && m_Device) {
            // Wait for GPU to finish before recreating swapchain
            m_Device->WaitIdle();

            // Recreate swapchain with new dimensions
            if (m_Swapchain && GetWindow()) {
                FirstEngine::RHI::SwapchainDescription swapchainDesc;
                swapchainDesc.width = width;
                swapchainDesc.height = height;
                m_Swapchain = m_Device->CreateSwapchain(GetWindow()->GetHandle(), swapchainDesc);
            }

            // Rebuild FrameGraph with new dimensions
            FirstEngine::Renderer::PipelineConfig config = 
                FirstEngine::Renderer::PipelineBuilder::CreateDeferredRenderingConfig(width, height);
            
            m_FrameGraph->Clear();
            FirstEngine::Renderer::PipelineBuilder builder(m_Device);
            builder.BuildFromConfig(config, *m_FrameGraph);
        }
    }

private:
    void InitializeRenderDoc() {
#ifdef _WIN32
        HMODULE mod = GetModuleHandleA("renderdoc.dll");
        if (mod) {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            if (RENDERDOC_GetAPI) {
                void* rdoc_api = nullptr;
                int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_2, &rdoc_api);
                if (ret == 1 && rdoc_api) {
                    m_RenderDocAPI = rdoc_api;
                    std::cout << "RenderDoc API initialized!" << std::endl;
                }
            }
        }
        // If RenderDoc is not available, that's okay - program will run normally
#endif
    }

    void BeginRenderDocFrame() {
#ifdef _WIN32
        if (m_RenderDocAPI) {
            // StartFrameCapture is at index 2 in the API structure
            typedef void (*StartFrameCaptureFunc)(void*, void*);
            void** api = (void**)m_RenderDocAPI;
            if (api && api[2]) {
                StartFrameCaptureFunc startFunc = (StartFrameCaptureFunc)api[2];
                startFunc(nullptr, nullptr);
            }
        }
#endif
    }

    void EndRenderDocFrame() {
#ifdef _WIN32
        if (m_RenderDocAPI) {
            // EndFrameCapture is at index 3 in the API structure
            typedef uint32_t (*EndFrameCaptureFunc)(void*, void*);
            void** api = (void**)m_RenderDocAPI;
            if (api && api[3]) {
                EndFrameCaptureFunc endFunc = (EndFrameCaptureFunc)api[3];
                endFunc(nullptr, nullptr);
            }
        }
#endif
    }

    FirstEngine::Device::VulkanDevice* m_Device;
    FirstEngine::Renderer::FrameGraph* m_FrameGraph;
    std::unique_ptr<FirstEngine::RHI::ISwapchain> m_Swapchain;
    FirstEngine::RHI::SemaphoreHandle m_ImageAvailableSemaphore;
    FirstEngine::RHI::SemaphoreHandle m_RenderFinishedSemaphore;
    FirstEngine::RHI::FenceHandle m_InFlightFence;
    std::unique_ptr<FirstEngine::RHI::ICommandBuffer> m_CommandBuffer; // Store command buffer to defer destruction
    std::chrono::high_resolution_clock::time_point m_LastTime;

#ifdef _WIN32
    void* m_RenderDocAPI = nullptr; // Opaque pointer to RenderDoc API
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
