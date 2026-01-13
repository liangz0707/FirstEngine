#include "FirstEngine/Core/Application.h"
#include <iostream>

class ExampleApp : public FirstEngine::Core::Application {
public:
    ExampleApp() : Application(1280, 720, "FirstEngine - Example") {}

protected:
    void OnUpdate(float deltaTime) override {
        // Update logic here
    }

    void OnRender() override {
        // Render logic here
    }

    void OnResize(int width, int height) override {
        std::cout << "Window resized to: " << width << "x" << height << std::endl;
    }
};

int main() {
    try {
        ExampleApp app;
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
