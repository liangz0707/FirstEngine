#include "FirstEngine/Core/Window.h"
#include <iostream>

namespace FirstEngine {
    namespace Core {
        Window::Window(int width, int height, const std::string& title)
            : m_Width(width), m_Height(height), m_Title(title), m_Window(nullptr) {
            
            if (!glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW");
            }

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

        Window::~Window() {
            if (m_Window) {
                glfwDestroyWindow(m_Window);
            }
            glfwTerminate();
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
