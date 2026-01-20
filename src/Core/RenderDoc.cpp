#include "FirstEngine/Core/RenderDoc.h"
#include <iostream>

#ifdef _WIN32

namespace FirstEngine {
    namespace Core {
        
        RENDERDOC_API_1_4_2* RenderDocHelper::s_RenderDocAPI = nullptr;
        
        void RenderDocHelper::Initialize() {
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
        
        void RenderDocHelper::BeginFrame() {
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
        
        void RenderDocHelper::EndFrame() {
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
        
    } // namespace Core
} // namespace FirstEngine

#endif // _WIN32
