#pragma once

#include "FirstEngine/Core/Export.h"
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace FirstEngine {
    namespace Core {
        class FE_CORE_API Window {
        public:
            using ResizeCallback = std::function<void(int, int)>;
            using KeyCallback = std::function<void(int, int, int, int)>;
            using MouseButtonCallback = std::function<void(int, int, int)>;
            using CursorPosCallback = std::function<void(double, double)>;

            Window(int width, int height, const std::string& title);
            ~Window();

            bool ShouldClose() const;
            void PollEvents();
            void SwapBuffers();

            GLFWwindow* GetHandle() const { return m_Window; }
            int GetWidth() const { return m_Width; }
            int GetHeight() const { return m_Height; }

            void SetResizeCallback(ResizeCallback callback);
            void SetKeyCallback(KeyCallback callback);
            void SetMouseButtonCallback(MouseButtonCallback callback);
            void SetCursorPosCallback(CursorPosCallback callback);

            bool IsKeyPressed(int key) const;

        private:
            GLFWwindow* m_Window;
            int m_Width;
            int m_Height;
            std::string m_Title;

            static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
            static void KeyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
            static void MouseButtonCallbackWrapper(GLFWwindow* window, int button, int action, int mods);
            static void CursorPosCallbackWrapper(GLFWwindow* window, double xpos, double ypos);

            ResizeCallback m_ResizeCallback;
            KeyCallback m_KeyCallback;
            MouseButtonCallback m_MouseButtonCallback;
            CursorPosCallback m_CursorPosCallback;
        };
    }
}
