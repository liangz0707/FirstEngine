#include "FirstEngine/Core/Application.h"
#include <chrono>
#include <iostream>

namespace FirstEngine {
    namespace Core {
        Application::Application(int width, int height, const std::string& title, bool headless)
            : m_LastFrameTime(0.0), m_Headless(headless) {
            
            if (!headless) {
                m_Window = std::make_unique<Window>(width, height, title);
                m_Window->SetResizeCallback([this](int w, int h) {
                    OnResize(w, h);
                });
            }
        }

        Application::~Application() {
            // Window will be destroyed automatically via unique_ptr
        }

        void Application::Run() {
            MainLoop();
        }

        void Application::MainLoop() {
            auto lastTime = std::chrono::high_resolution_clock::now();

            while (m_Headless || (m_Window && !m_Window->ShouldClose())) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
                float deltaTime = duration.count() / 1000000.0f;
                lastTime = currentTime;
                m_LastFrameTime = deltaTime;

                if (m_Window) {
                    m_Window->PollEvents();
                }

                OnUpdate(deltaTime);
                OnPrepareFrameGraph(); // Build FrameGraph execution plan (before OnRender)
                OnRender();  // Collect render data
                OnSubmit();  // Submit command buffers
            }
        }
    }
}
