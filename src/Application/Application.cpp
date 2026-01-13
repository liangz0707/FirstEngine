#include "FirstEngine/Core/Application.h"
#include <chrono>
#include <iostream>

namespace FirstEngine {
    namespace Core {
        Application::Application(int width, int height, const std::string& title)
            : m_LastFrameTime(0.0) {
            
            m_Window = std::make_unique<Window>(width, height, title);
            m_Renderer = std::make_unique<Device::VulkanRenderer>(m_Window.get());

            m_Window->SetResizeCallback([this](int w, int h) {
                m_Renderer->OnWindowResize(w, h);
                OnResize(w, h);
            });
        }

        Application::~Application() {
            // Renderer and Window will be destroyed automatically via unique_ptr
        }

        void Application::Run() {
            MainLoop();
        }

        void Application::MainLoop() {
            auto lastTime = std::chrono::high_resolution_clock::now();

            while (!m_Window->ShouldClose()) {
                auto currentTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
                float deltaTime = duration.count() / 1000000.0f;
                lastTime = currentTime;

                m_Window->PollEvents();

                OnUpdate(deltaTime);
                OnRender();
                m_Renderer->Present();
            }

            m_Renderer->WaitIdle();
        }
    }
}
