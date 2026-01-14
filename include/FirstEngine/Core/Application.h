#pragma once

#include "FirstEngine/Core/Window.h"
#include <memory>
#include <string>

namespace FirstEngine {
    namespace Core {
        class Application {
        public:
            Application(int width = 1280, int height = 720, const std::string& title = "FirstEngine", bool headless = false);
            virtual ~Application();

            void Run();
            virtual bool Initialize() { return true; }

            Window* GetWindow() const { return m_Window.get(); }

        protected:
            virtual void OnUpdate(float deltaTime) {}
            virtual void OnRender() {}
            virtual void OnResize(int width, int height) {}

            std::unique_ptr<Window> m_Window;
            bool m_Headless;

        private:
            void MainLoop();
            double m_LastFrameTime;
        };
    }
}
