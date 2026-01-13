#pragma once

#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Device/VulkanRenderer.h"
#include <memory>

namespace FirstEngine {
    namespace Core {
        class Application {
        public:
            Application(int width = 1280, int height = 720, const std::string& title = "FirstEngine");
            ~Application();

            void Run();

        protected:
            virtual void OnUpdate(float deltaTime) {}
            virtual void OnRender() {}
            virtual void OnResize(int width, int height) {}

            std::unique_ptr<Window> m_Window;
            std::unique_ptr<Device::VulkanRenderer> m_Renderer;

        private:
            void MainLoop();
            double m_LastFrameTime;
        };
    }
}
