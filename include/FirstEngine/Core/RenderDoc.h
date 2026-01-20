#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#include <iostream>

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

namespace FirstEngine {
    namespace Core {
        // RenderDoc 工具类 - 提供统一的 RenderDoc 集成
        // 实现放在头文件中以避免循环依赖（EditorAPI 只需要头文件，不需要链接 FirstEngine_Core）
        class RenderDocHelper {
        public:
            // 初始化 RenderDoc（必须在创建 Vulkan 实例之前调用）
            static void Initialize() {
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
                            std::cout << "RenderDoc: Loaded from: " << renderdocPaths[i] << std::endl;
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
                                s_RenderDocAPI = rdoc_api;
                                std::cout << "RenderDoc: API initialized successfully!" << std::endl;
                                
                                // Set capture options (optional)
                                if (rdoc_api->SetCaptureOptionU32) {
                                    rdoc_api->SetCaptureOptionU32(1, 1); // Allow VSync
                                    rdoc_api->SetCaptureOptionU32(2, 0); // Allow fullscreen
                                }
                                
                                // Check if RenderDoc is connected
                                if (rdoc_api->IsTargetControlConnected && rdoc_api->IsTargetControlConnected()) {
                                    std::cout << "RenderDoc: Target control is connected!" << std::endl;
                                } else {
                                    std::cout << "RenderDoc: Target control is NOT connected. Make sure RenderDoc UI is running." << std::endl;
                                }
                            } else {
                                std::cout << "RenderDoc: API structure is invalid (missing function pointers)." << std::endl;
                            }
                        } else {
                            std::cout << "RenderDoc: Failed to get API. Return code: " << ret << std::endl;
                        }
                    } else {
                        std::cout << "RenderDoc: Failed to get RENDERDOC_GetAPI function pointer." << std::endl;
                    }
                } else {
                    std::cout << "RenderDoc: DLL not found. RenderDoc capture will not be available." << std::endl;
                    std::cout << "RenderDoc: To use RenderDoc:" << std::endl;
                    std::cout << "RenderDoc:   1. Install RenderDoc from https://renderdoc.org/" << std::endl;
                    std::cout << "RenderDoc:   2. Launch this application from RenderDoc UI (Launch Application)" << std::endl;
                    std::cout << "RenderDoc:   3. Or inject RenderDoc into this process (Inject into Process)" << std::endl;
                }
                // If RenderDoc is not available, that's okay - program will run normally
            }
            
            // 开始帧捕获（在帧开始时调用）
            static void BeginFrame() {
                if (s_RenderDocAPI) {
                    // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
                    // This is the safest approach and avoids access violation errors
                    if (s_RenderDocAPI->StartFrameCapture) {
                        try {
                            s_RenderDocAPI->StartFrameCapture(nullptr, nullptr);
                        } catch (...) {
                            // Silently ignore errors - RenderDoc might not be fully initialized yet
                        }
                    }
                }
            }
            
            // 结束帧捕获（在帧提交后调用）
            static void EndFrame() {
                if (s_RenderDocAPI) {
                    // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
                    if (s_RenderDocAPI->EndFrameCapture) {
                        try {
                            s_RenderDocAPI->EndFrameCapture(nullptr, nullptr);
                        } catch (...) {
                            // Silently ignore errors - RenderDoc might not be fully initialized yet
                        }
                    }
                }
            }
            
            // 检查 RenderDoc 是否可用
            static bool IsAvailable() { return s_RenderDocAPI != nullptr; }
            
        private:
            // 使用 inline 关键字（C++17）在头文件中定义静态成员变量
            inline static RENDERDOC_API_1_4_2* s_RenderDocAPI = nullptr;
        };
    }
}

#endif // _WIN32
