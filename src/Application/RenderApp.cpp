#include "FirstEngine/Core/Application.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Core/CommandLine.h"
#include "FirstEngine/Core/RenderDoc.h"
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
#include "FirstEngine/Renderer/RenderContext.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
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
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

class RenderApp : public FirstEngine::Core::Application {
public:
    RenderApp(int width, int height, const std::string& title, bool headless)
        : Application(width, height, title, headless),
          m_pRenderContext(nullptr),
          m_LastTime(std::chrono::high_resolution_clock::now()) {
    }

    ~RenderApp() override {
        // Wait for GPU to complete
        if (m_pRenderContext && m_pRenderContext->GetDevice()) {
            m_pRenderContext->GetDevice()->WaitIdle();
        }

        // Destroy swapchain (window-related, managed by RenderApp)
        m_Swapchain.reset();

        // Shutdown and destroy RenderContext (it will clean up all internally managed resources)
        if (m_pRenderContext) {
            m_pRenderContext->ShutdownEngine();
            delete m_pRenderContext;
            m_pRenderContext = nullptr;
        }
    }

    bool Initialize() override {
        // Initialize RenderDoc BEFORE creating Vulkan instance
        // This is critical - RenderDoc needs to intercept Vulkan calls from the start
#ifdef _WIN32
        FirstEngine::Core::RenderDocHelper::Initialize();
#endif

        // Create RenderContext instance
        m_pRenderContext = new FirstEngine::Renderer::RenderContext();

        // Get window handle
        void* windowHandle = GetWindow() ? GetWindow()->GetHandle() : nullptr;
        if (!windowHandle) {
            std::cerr << "Error: No window handle available!" << std::endl;
            return false;
        }

        // Get window dimensions
        int width = GetWindow() ? GetWindow()->GetWidth() : 1280;
        int height = GetWindow() ? GetWindow()->GetHeight() : 720;

        // Use RenderContext::InitializeForWindow to initialize rendering engine
        // This creates device, pipeline, frameGraph, sync objects, scene, etc.
        if (!m_pRenderContext->InitializeForWindow(windowHandle, width, height)) {
            std::cerr << "Failed to initialize RenderContext!" << std::endl;
            return false;
        }

        // Create swapchain (window-related, managed by RenderApp)
        if (GetWindow()) {
            const auto& resolution = m_pRenderContext->GetRenderConfig().GetResolution();
            FirstEngine::RHI::SwapchainDescription swapchainDesc;
            swapchainDesc.width = resolution.width;
            swapchainDesc.height = resolution.height;
            m_Swapchain = m_pRenderContext->GetDevice()->CreateSwapchain(windowHandle, swapchainDesc);
            if (!m_Swapchain) {
                std::cerr << "Failed to create swapchain!" << std::endl;
                return false;
            }
        }

        // Load scene (if exists)
        FirstEngine::Resources::ResourceManager& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();

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
            if (resourceManager.LoadManifest(manifestPath)) {
                std::cout << "Loaded resource manifest from: " << manifestPath << std::endl;
            } else {
                std::cerr << "Failed to load resource manifest!" << std::endl;
            }
        } else {
            std::cout << "Resource manifest not found: " << manifestPath << std::endl;
            std::cout << "Will register resources on demand." << std::endl;
        }

        // Load scene (if exists)
        std::string scenePath = resolvePath("build/Package/Scenes/example_scene.json");
        if (fs::exists(scenePath)) {
            if (m_pRenderContext->LoadScene(scenePath)) {
                std::cout << "Loaded scene from: " << scenePath << std::endl;
            } else {
                std::cerr << "Failed to load scene from: " << scenePath << std::endl;
            }
        } else {
            std::cerr << "Scene file not found: " << scenePath << std::endl;
            std::cerr << "Current working directory: " << fs::current_path().string() << std::endl;
        }

        std::cout << "RenderApp initialized successfully!" << std::endl;
        return true;
    }

protected:
    void OnUpdate(float deltaTime) override {
        // Update logic here
    }

    void OnPrepareFrameGraph() override {
        if (!m_pRenderContext || !m_pRenderContext->IsEngineInitialized()) {
            return;
        }

        // Begin RendRenderDocHelpererDoc frame capture
#ifdef _WIN32
        FirstEngine::Core::RenderDocHelper::BeginFrame();
#endif

        // Use RenderContext to build FrameGraph (all parameters are obtained internally)
        if (!m_pRenderContext->BeginFrame()) {
            std::cerr << "Failed to begin frame in RenderContext!" << std::endl;
            return;
        }
    }

    void OnCreateResources() override {
        if (!m_pRenderContext || !m_pRenderContext->IsEngineInitialized()) {
            return;
        }

        // Use RenderContext to process resources (create/update operations only)
        // Destroy operations are handled in SubmitFrame after frame submission
        // This ensures resources are not destroyed while still in use by command buffers
        // Note: ProcessResources now handles create/update and destroy separately
        m_pRenderContext->ProcessResources(nullptr, 0);
    }

    void OnRender() override {
        if (!m_pRenderContext || !m_pRenderContext->IsEngineInitialized()) {
            return;
        }

        // Update render config (if window size changed)
        if (GetWindow()) {
            m_pRenderContext->SetRenderConfig(
                GetWindow()->GetWidth(),
                GetWindow()->GetHeight(),
                1.0f
            );
        }

        // Use RenderContext to execute FrameGraph (all parameters are obtained internally)
        if (!m_pRenderContext->ExecuteFrameGraph()) {
            std::cerr << "Failed to execute FrameGraph in RenderContext!" << std::endl;
            return;
        }
    }

    void OnSubmit() override {
        if (!m_pRenderContext || !m_pRenderContext->IsEngineInitialized() || !m_Swapchain) {
            return;
        }

        // Use RenderContext to submit rendering (only need to provide swapchain, everything else is obtained internally)
        FirstEngine::Renderer::RenderContext::RenderParams params;
        params.swapchain = m_Swapchain.get();
        
        if (!m_pRenderContext->SubmitFrame(params)) {
            // Submit failed (may be image acquisition failure or need to recreate swapchain)
            return;
        }

        // End RenderDoc frame capture (after frame is submitted)
#ifdef _WIN32
        FirstEngine::Core::RenderDocHelper::EndFrame();
#endif
    }

    void OnResize(int width, int height) override {
        // Ignore resize events with invalid dimensions (window may be minimized)
        if (width <= 0 || height <= 0) {
            return;
        }

        if (!m_pRenderContext || !m_pRenderContext->IsEngineInitialized()) {
            return;
        }

        // Update RenderContext's render config
        m_pRenderContext->SetRenderConfig(width, height, 1.0f);

        // Wait for GPU to complete
        auto* device = m_pRenderContext->GetDevice();
        if (device) {
            device->WaitIdle();

            // Recreate swapchain (window-related, managed by RenderApp)
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
                // If swapchain doesn't exist, create it
                try {
                    FirstEngine::RHI::SwapchainDescription swapchainDesc;
                    swapchainDesc.width = width;
                    swapchainDesc.height = height;
                    m_Swapchain = device->CreateSwapchain(GetWindow()->GetHandle(), swapchainDesc);
                    
                    if (!m_Swapchain) {
                        std::cerr << "Failed to create swapchain on resize!" << std::endl;
                        return;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error creating swapchain: " << e.what() << std::endl;
                    return;
                }
            }
        }
    }

private:
    // RenderContext pointer (manages all rendering-related resources: device, pipeline, frameGraph, scene, sync objects, etc.)
    FirstEngine::Renderer::RenderContext* m_pRenderContext = nullptr;
    
    // Swapchain (window-related, managed by RenderApp)
    std::unique_ptr<FirstEngine::RHI::ISwapchain> m_Swapchain;
    
    std::chrono::high_resolution_clock::time_point m_LastTime;
};

int main(int argc, char* argv[]) {
    FirstEngine::Core::CommandLineParser parser;

    // Mode selection
    parser.AddArgument("editor", "e", "Launch editor mode", FirstEngine::Core::ArgumentType::Flag);
    parser.AddArgument("standalone", "s", "Launch standalone render window (default)", FirstEngine::Core::ArgumentType::Flag);
    
    // Window parameters (for standalone mode)
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
        std::cout << "\nModes:\n";
        std::cout << "  --editor, -e     Launch C# editor (WPF application)\n";
        std::cout << "  --standalone, -s Launch standalone render window (default)\n";
        return 0;
    }

    // Determine mode
    bool editorMode = parser.GetBool("editor");
    bool standaloneMode = parser.GetBool("standalone");
    
    // Default to standalone if neither is specified
    if (!editorMode && !standaloneMode) {
        standaloneMode = true;
    }

    if (editorMode) {
        // Launch editor
        // On Windows, we'll launch the C# editor executable
        #ifdef _WIN32
        std::string editorPath = "FirstEngineEditor.exe";
        
        // Try to find editor in common locations
        char exePath[MAX_PATH];
        DWORD pathLen = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string foundEditorPath;
        
        if (pathLen > 0 && pathLen < MAX_PATH) {
            // Get directory
            std::string exeDir;
            for (int i = pathLen - 1; i >= 0; i--) {
                if (exePath[i] == '\\' || exePath[i] == '/') {
                    exePath[i + 1] = '\0';
                    exeDir = exePath;
                    break;
                }
            }
            
            // List of paths to try (relative to executable directory)
            std::vector<std::string> searchPaths = {
                exeDir + editorPath,                                    // Same directory
                exeDir + "../" + editorPath,                            // Parent directory
                exeDir + "../../../Editor/bin/Debug/net8.0-windows/" + editorPath,  // Editor Debug (from build/bin/Debug, go up 3 levels)
                exeDir + "../../../Editor/bin/Release/net8.0-windows/" + editorPath, // Editor Release
                exeDir + "../../Editor/bin/Debug/net8.0-windows/" + editorPath,     // Editor Debug (from build/bin, go up 2 levels)
                exeDir + "../../Editor/bin/Release/net8.0-windows/" + editorPath,   // Editor Release
            };
            
            // Try each path
            for (const auto& path : searchPaths) {
                // Normalize path (convert / to \)
                std::string normalizedPath = path;
                std::replace(normalizedPath.begin(), normalizedPath.end(), '/', '\\');
                
                // Resolve .. using Windows API for proper path resolution
                char resolvedPath[MAX_PATH];
                DWORD result = GetFullPathNameA(normalizedPath.c_str(), MAX_PATH, resolvedPath, nullptr);
                if (result > 0 && result < MAX_PATH) {
                    normalizedPath = resolvedPath;
                } else {
                    // Fallback: Simple .. resolution
                    size_t pos;
                    while ((pos = normalizedPath.find("\\..\\")) != std::string::npos) {
                        size_t prevSlash = normalizedPath.rfind('\\', pos - 1);
                        if (prevSlash != std::string::npos) {
                            normalizedPath.erase(prevSlash, pos + 4 - prevSlash);
                        } else {
                            break;
                        }
                    }
                }
                
                // Check if file exists
                DWORD fileAttributes = GetFileAttributesA(normalizedPath.c_str());
                if (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    foundEditorPath = normalizedPath;
                    break;
                }
            }
            
            // If still not found, try to find Editor directory by going up from exe directory
            if (foundEditorPath.empty()) {
                std::string currentPath = exeDir;
                
                // Go up directories to find Editor folder (max 5 levels up)
                for (int i = 0; i < 5; ++i) {
                    // Try Debug
                    std::string testPath = currentPath + "\\Editor\\bin\\Debug\\net8.0-windows\\" + editorPath;
                    DWORD attrs = GetFileAttributesA(testPath.c_str());
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        foundEditorPath = testPath;
                        break;
                    }
                    
                    // Try Release
                    testPath = currentPath + "\\Editor\\bin\\Release\\net8.0-windows\\" + editorPath;
                    attrs = GetFileAttributesA(testPath.c_str());
                    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                        foundEditorPath = testPath;
                        break;
                    }
                    
                    // Go up one directory level
                    size_t lastSlash = currentPath.find_last_of("\\/");
                    if (lastSlash != std::string::npos && lastSlash > 0) {
                        currentPath = currentPath.substr(0, lastSlash);
                    } else {
                        break;
                    }
                }
            }
        }
        
        if (!foundEditorPath.empty()) {
            // Launch editor
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            std::string cmdLine = "\"" + foundEditorPath + "\"";
            if (CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                // Don't close handles immediately - wait for process to exit
                // This prevents the debugger from detaching when the parent process exits
                CloseHandle(pi.hThread);
                
                // Wait for the editor process to exit
                // This keeps the parent process alive so the debugger stays attached
                WaitForSingleObject(pi.hProcess, INFINITE);
                
                // Get exit code
                DWORD exitCode = 0;
                GetExitCodeProcess(pi.hProcess, &exitCode);
                CloseHandle(pi.hProcess);
                
                return static_cast<int>(exitCode);
            } else {
                std::cerr << "Error: Failed to launch editor: " << foundEditorPath << std::endl;
                std::cerr << "Error code: " << GetLastError() << std::endl;
                return 1;
            }
        }
        
        std::cerr << "Error: Could not find FirstEngineEditor.exe" << std::endl;
        std::cerr << "Searched in:" << std::endl;
        std::cerr << "  - Same directory as FirstEngine.exe" << std::endl;
        std::cerr << "  - Parent directory" << std::endl;
        std::cerr << "  - Editor/bin/Debug/net8.0-windows/" << std::endl;
        std::cerr << "  - Editor/bin/Release/net8.0-windows/" << std::endl;
        std::cerr << "Please ensure the editor is built: cd Editor && dotnet build FirstEngineEditor.csproj -c Debug" << std::endl;
        return 1;
        #else
        std::cerr << "Error: Editor mode is only supported on Windows" << std::endl;
        return 1;
        #endif
    } else {
        // Standalone mode - run render window directly
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
    }

    return 0;
}
