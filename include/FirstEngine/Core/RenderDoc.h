#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#include <winnt.h>  // For SEH support
#include <iostream>
#include <stdlib.h>  // For _dupenv_s
#include <string>
#include <debugapi.h>  // For OutputDebugStringA

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
        // RenderDoc utility class - provides unified RenderDoc integration
        // Implementation is in header file to avoid circular dependency (EditorAPI only needs header, doesn't need to link FirstEngine_Core)
        class RenderDocHelper {
        public:
            // Initialize RenderDoc (must be called before creating Vulkan instance)
            static void Initialize() {
                // Check environment variable to force enable RenderDoc even in debug mode
                // Set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable RenderDoc in debug builds
                bool forceEnable = false;
                char* envValue = nullptr;
                size_t envSize = 0;
                if (_dupenv_s(&envValue, &envSize, "FIRSTENGINE_ENABLE_RENDERDOC") == 0 && envValue) {
                    forceEnable = (std::string(envValue) == "1" || std::string(envValue) == "true");
                    free(envValue);
                }
                
                // Skip RenderDoc initialization in debug mode unless forced
                #ifdef _DEBUG
                if (!forceEnable) {
                    std::cout << "RenderDoc: Skipping initialization in debug mode (set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable)" << std::endl;
                    return;
                } else {
                    std::cout << "RenderDoc: Force enabled via FIRSTENGINE_ENABLE_RENDERDOC environment variable" << std::endl;
                }
                #endif
                
                // Check if debugger is attached at runtime (only skip if not forced)
                if (IsDebuggerPresent() && !forceEnable) {
                    std::cout << "RenderDoc: Skipping initialization - debugger is attached (set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable)" << std::endl;
                    return;
                }
                
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
            
            // Begin frame capture (called at frame start)
            static void BeginFrame() {
                // Check environment variable to force enable RenderDoc even in debug mode
                bool forceEnable = CheckForceEnable();
                
                // Skip RenderDoc in debug mode unless forced
                #ifdef _DEBUG
                if (!forceEnable) {
                    return;
                }
                #endif
                
                // Check if debugger is attached at runtime (only skip if not forced)
                if (IsDebuggerPresent() && !forceEnable) {
                    return;
                }
                
                if (s_RenderDocAPI) {
                    // Verify the API pointer is still valid
                    if (!IsValidPointer(s_RenderDocAPI)) {
                        s_RenderDocAPI = nullptr;
                        return;
                    }
                    
                    // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
                    // This is the safest approach and avoids access violation errors
                    if (s_RenderDocAPI->StartFrameCapture) {
                        // Call helper function that uses SEH (no object unwinding in that function)
                        StartFrameCaptureSafe();
                    }
                }
            }
            
            // End frame capture (called after frame submission)
            static void EndFrame() {
                // Check environment variable to force enable RenderDoc even in debug mode
                bool forceEnable = CheckForceEnable();
                
                // Skip RenderDoc in debug mode unless forced
                #ifdef _DEBUG
                if (!forceEnable) {
                    return;
                }
                #endif
                
                // Check if debugger is attached at runtime (only skip if not forced)
                if (IsDebuggerPresent() && !forceEnable) {
                    return;
                }
                
                if (s_RenderDocAPI) {
                    // Verify the API pointer is still valid
                    if (!IsValidPointer(s_RenderDocAPI)) {
                        s_RenderDocAPI = nullptr;
                        return;
                    }
                    
                    // Pass nullptr for both parameters - RenderDoc will automatically capture all devices/windows
                    if (s_RenderDocAPI->EndFrameCapture) {
                        // Call helper function that uses SEH (no object unwinding in that function)
                        EndFrameCaptureSafe();
                    }
                }
            }
            
            // Check if RenderDoc is available
            static bool IsAvailable() { 
                // Check environment variable to force enable RenderDoc even in debug mode
                bool forceEnable = CheckForceEnable();
                
                #ifdef _DEBUG
                if (!forceEnable) {
                    return false; // Disable in debug mode unless forced
                }
                #endif
                
                // Disable if debugger is attached (unless forced)
                if (IsDebuggerPresent() && !forceEnable) {
                    return false;
                }
                
                return s_RenderDocAPI != nullptr && IsValidPointer(s_RenderDocAPI);
            }
            
        private:
            // Check if RenderDoc is force enabled via environment variable
            // This function handles object unwinding (std::string), so it cannot be in __try block
            static bool CheckForceEnable() {
                bool forceEnable = false;
                char* envValue = nullptr;
                size_t envSize = 0;
                if (_dupenv_s(&envValue, &envSize, "FIRSTENGINE_ENABLE_RENDERDOC") == 0 && envValue) {
                    forceEnable = (std::string(envValue) == "1" || std::string(envValue) == "true");
                    free(envValue);
                }
                return forceEnable;
            }
            
            // Safe wrapper for StartFrameCapture using SEH
            // This function has no object unwinding, so it can use __try/__except
            static void StartFrameCaptureSafe() {
                __try {
                    if (s_RenderDocAPI && s_RenderDocAPI->StartFrameCapture) {
                        s_RenderDocAPI->StartFrameCapture(nullptr, nullptr);
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    // Access violation or other exception occurred
                    // Disable RenderDoc to prevent further crashes
                    s_RenderDocAPI = nullptr;
                    // Note: Cannot use std::cout here (object unwinding), use OutputDebugStringA instead
                    OutputDebugStringA("RenderDoc: Exception in StartFrameCapture, disabling RenderDoc\n");
                }
            }
            
            // Safe wrapper for EndFrameCapture using SEH
            // This function has no object unwinding, so it can use __try/__except
            static void EndFrameCaptureSafe() {
                __try {
                    if (s_RenderDocAPI && s_RenderDocAPI->EndFrameCapture) {
                        s_RenderDocAPI->EndFrameCapture(nullptr, nullptr);
                    }
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    // Access violation or other exception occurred
                    // Disable RenderDoc to prevent further crashes
                    s_RenderDocAPI = nullptr;
                    // Note: Cannot use std::cout here (object unwinding), use OutputDebugStringA instead
                    OutputDebugStringA("RenderDoc: Exception in EndFrameCapture, disabling RenderDoc\n");
                }
            }
            
            // Verify pointer is valid (avoid access violation)
            static bool IsValidPointer(void* ptr) {
                if (!ptr) return false;
                
                // Use VirtualQuery to check if we can safely read from this pointer
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
                    // Check if the memory is committed and readable
                    return (mbi.State == MEM_COMMIT) && 
                           (mbi.Protect == PAGE_READONLY || 
                            mbi.Protect == PAGE_READWRITE || 
                            mbi.Protect == PAGE_EXECUTE_READ || 
                            mbi.Protect == PAGE_EXECUTE_READWRITE ||
                            mbi.Protect == PAGE_WRITECOPY ||
                            mbi.Protect == PAGE_EXECUTE_WRITECOPY);
                }
                return false;
            }
            
            // Use inline keyword (C++17) to define static member variable in header file
            inline static RENDERDOC_API_1_4_2* s_RenderDocAPI = nullptr;
        };
    }
}

#endif // _WIN32

