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
          m_LastTime(std::chrono::high_resolution_clock::now()) {
    }

    ~RenderApp() override {
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

        std::cout << "RenderApp initialized successfully!" << std::endl;
        return true;
    }

protected:
    void OnUpdate(float deltaTime) override {
        // Update logic here
    }

    void OnRender() override {
        if (!m_Device || !m_FrameGraph) {
            return;
        }

        // Begin RenderDoc frame capture
        BeginRenderDocFrame();

        // Create command buffer
        std::unique_ptr<FirstEngine::RHI::ICommandBuffer> commandBuffer = m_Device->CreateCommandBuffer();
        if (!commandBuffer) {
            return;
        }

        // Begin recording
        commandBuffer->Begin();

        // Execute FrameGraph
        m_FrameGraph->Execute(commandBuffer.get());

        // End recording
        commandBuffer->End();

        // Submit command buffer
        auto semaphore = m_Device->CreateSemaphore();
        std::vector<FirstEngine::RHI::SemaphoreHandle> signalSemaphores = {semaphore};
        m_Device->SubmitCommandBuffer(commandBuffer.get(), {}, signalSemaphores, nullptr);

        // Present (if swapchain exists and we have a window)
        if (GetWindow()) {
            FirstEngine::RHI::SwapchainDescription swapchainDesc;
            swapchainDesc.width = GetWindow()->GetWidth();
            swapchainDesc.height = GetWindow()->GetHeight();
            auto swapchain = m_Device->CreateSwapchain(GetWindow()->GetHandle(), swapchainDesc);
            if (swapchain) {
                uint32_t imageIndex;
                if (swapchain->AcquireNextImage(semaphore, nullptr, imageIndex)) {
                    std::vector<FirstEngine::RHI::SemaphoreHandle> presentWaitSemaphores = {semaphore};
                    swapchain->Present(imageIndex, presentWaitSemaphores);
                }
            }
        }

        // End RenderDoc frame capture
        EndRenderDocFrame();

        m_Device->DestroySemaphore(semaphore);
    }

    void OnResize(int width, int height) override {
        if (m_FrameGraph && m_Device) {
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
