#include "FirstEngine/Core/Window.h"
#include <iostream>
#include <vector>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

// Vulkan types (needed for PFN_vkGetInstanceProcAddr)
// GLFW_INCLUDE_VULKAN is already defined in Window.h, but we need the types here
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace FirstEngine {
    namespace Core {
        
        // Helper function to pre-load Vulkan DLL
        // GLFW 3.3.8 will search for vulkan-1.dll using LoadLibrary's search order
        // By pre-loading it, we ensure it's available when GLFW checks for Vulkan support
        static PFN_vkGetInstanceProcAddr LoadVulkanLoader() {
#ifdef _WIN32
            // Get executable directory for relative paths
            char exePath[MAX_PATH];
            DWORD pathLen = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            std::string exeDir;
            if (pathLen > 0 && pathLen < MAX_PATH) {
                // Remove executable name, keep directory
                for (int i = pathLen - 1; i >= 0; i--) {
                    if (exePath[i] == '\\' || exePath[i] == '/') {
                        exePath[i + 1] = '\0';
                        exeDir = exePath;
                        break;
                    }
                }
            }
            
            // Try multiple locations for vulkan-1.dll (in order of preference)
            std::vector<std::string> dllPaths;
            
            // 1. Same directory as executable (most reliable - CMake copies it here)
            if (!exeDir.empty()) {
                dllPaths.push_back(exeDir + "vulkan-1.dll");
            }
            
            // 2. Current directory or PATH
            dllPaths.push_back("vulkan-1.dll");
            
            // 3. System directory (absolute path - always works)
            dllPaths.push_back("C:/Windows/System32/vulkan-1.dll");
            
            // 4. Relative to executable (project structure)
            if (!exeDir.empty()) {
                dllPaths.push_back(exeDir + "../external/vulkan/vulkan-1.dll");
                dllPaths.push_back(exeDir + "../../external/vulkan/vulkan-1.dll");
            }
            
            HMODULE vulkanModule = nullptr;
            std::string loadedFrom;
            
            for (const auto& path : dllPaths) {
                vulkanModule = LoadLibraryA(path.c_str());
                if (vulkanModule) {
                    loadedFrom = path;
                    break;
                }
            }
            
            if (vulkanModule) {
                PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = 
                    (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanModule, "vkGetInstanceProcAddr");
                if (vkGetInstanceProcAddr) {
                    std::cout << "Successfully pre-loaded vulkan-1.dll from: " << loadedFrom << std::endl;
                    // Note: We don't FreeLibrary because GLFW needs it to remain loaded
                    return vkGetInstanceProcAddr;
                } else {
                    std::cerr << "Warning: vulkan-1.dll loaded but vkGetInstanceProcAddr not found" << std::endl;
                    FreeLibrary(vulkanModule);
                }
            } else {
                DWORD error = GetLastError();
                std::cerr << "Warning: Could not pre-load vulkan-1.dll (Error: " << error << ")" << std::endl;
                std::cerr << "  Tried " << dllPaths.size() << " locations:" << std::endl;
                for (const auto& path : dllPaths) {
                    std::cerr << "    - " << path << std::endl;
                }
            }
#endif
            return nullptr;
        }
        
        Window::Window(int width, int height, const std::string& title)
            : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr), m_OwnsGLFW(true) {
            
            // Pre-load Vulkan DLL to ensure it's available when GLFW checks for Vulkan support
            // GLFW 3.3.8 doesn't have glfwInitVulkanLoader, so we just ensure the DLL is loaded
            // GLFW will find it via LoadLibrary's search order
            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = LoadVulkanLoader();
            if (vkGetInstanceProcAddr) {
                std::cout << "Pre-loaded Vulkan DLL, GLFW should be able to find it" << std::endl;
            } else {
                std::cout << "Warning: Could not pre-load Vulkan DLL, GLFW will search for it" << std::endl;
            }
            
            if (!glfwInit()) {
                const char* errorDescription = nullptr;
                int errorCode = glfwGetError(&errorDescription);
                std::string errorMsg = "Failed to initialize GLFW";
                if (errorDescription) {
                    errorMsg += " (Error Code: ";
                    errorMsg += std::to_string(errorCode);
                    errorMsg += ", Description: ";
                    errorMsg += errorDescription;
                    errorMsg += ")";
                }
                throw std::runtime_error(errorMsg);
            }

            // Set window hints for Vulkan (no OpenGL context)
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

            m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
            if (!m_Window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            glfwSetWindowUserPointer(m_Window, this);
            glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
            glfwSetKeyCallback(m_Window, KeyCallbackWrapper);
            glfwSetMouseButtonCallback(m_Window, MouseButtonCallbackWrapper);
            glfwSetCursorPosCallback(m_Window, CursorPosCallbackWrapper);
        }

        Window::Window(GLFWwindow* existingWindow)
            : m_Window(existingWindow), m_OwnsGLFW(false) {
            if (!existingWindow) {
                throw std::runtime_error("Cannot create Window from null GLFWwindow");
            }

            // Get window properties from existing window
            glfwGetWindowSize(existingWindow, &m_Width, &m_Height);
            
            m_Title ="FirstEngine";

            // Don't overwrite user pointer if it's already set (by Application's Window)
            // This ensures the original Window object's callbacks still work
            void* existingPointer = glfwGetWindowUserPointer(existingWindow);
            if (!existingPointer) {
                glfwSetWindowUserPointer(m_Window, this);
            }
            
            // Note: We don't set callbacks here to avoid overwriting Application's callbacks
            // The existing Window object (created by Application) already has its callbacks set
            // This Window wrapper is only used internally by VulkanDevice/VulkanRenderer
        }

        Window::~Window() {
            if (m_Window && m_OwnsGLFW) {
                glfwDestroyWindow(m_Window);
                glfwTerminate();
            }
            // If we don't own GLFW, we don't destroy the window or terminate GLFW
        }

        bool Window::ShouldClose() const {
            return glfwWindowShouldClose(m_Window);
        }

        void Window::PollEvents() {
            glfwPollEvents();
        }

        void Window::SwapBuffers() {
            // For Vulkan, swapchain presentation is handled separately
        }

        void Window::SetResizeCallback(ResizeCallback callback) {
            m_ResizeCallback = callback;
        }

        void Window::SetKeyCallback(KeyCallback callback) {
            m_KeyCallback = callback;
        }

        void Window::SetMouseButtonCallback(MouseButtonCallback callback) {
            m_MouseButtonCallback = callback;
        }

        void Window::SetCursorPosCallback(CursorPosCallback callback) {
            m_CursorPosCallback = callback;
        }

        bool Window::IsKeyPressed(int key) const {
            return glfwGetKey(m_Window, key) == GLFW_PRESS;
        }

        void Window::GetFramebufferSize(int* width, int* height) const {
            glfwGetFramebufferSize(m_Window, width, height);
        }

        void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win) {
                win->m_Width = width;
                win->m_Height = height;
                if (win->m_ResizeCallback) {
                    win->m_ResizeCallback(width, height);
                }
            }
        }

        void Window::KeyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_KeyCallback) {
                win->m_KeyCallback(key, scancode, action, mods);
            }
        }

        void Window::MouseButtonCallbackWrapper(GLFWwindow* window, int button, int action, int mods) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_MouseButtonCallback) {
                win->m_MouseButtonCallback(button, action, mods);
            }
        }

        void Window::CursorPosCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
            Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_CursorPosCallback) {
                win->m_CursorPosCallback(xpos, ypos);
            }
        }
    }
}
