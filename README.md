# FirstEngine - Modern 3D Graphics Engine

A modern 3D graphics engine built with C++, Vulkan, and GLFW.

## Features

- **Core Module**: Window management and main application loop
- **Renderer Module**: Vulkan initialization, swapchain, and graphics pipeline
- **Resources Module**: Shader and texture loading utilities
- **Math Support**: GLM library integration

## Requirements

- C++17 compatible compiler
- CMake 3.20 or higher
- Vulkan SDK
- Git（用于初始化子模块）

> 说明：GLFW / GLM / glslang / SPIRV-Cross / stb / assimp / pybind11 等第三方库以 **Git 子模块**形式提供，位于 `third_party/`，无需你额外安装。

## Building

### 获取源码（务必初始化子模块）

首次克隆后先执行：

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

如果你这里遇到“网络问题/超时/连接重置”，可以用带重试的脚本：

```bash
bash scripts/init_submodules.sh
```

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

编译完成后，动态库和可执行文件将位于 `build/bin/Release/` 目录。

### 独立编译模块

每个模块可以独立编译：

```bash
# 只编译Shader模块
cmake --build . --target FirstEngine_Shader

# 只编译Renderer模块（会自动编译依赖）
cmake --build . --target FirstEngine_Renderer
```

## Project Structure

项目已重构为模块化的动态链接库架构，每个模块可以独立开发和编译。详细说明请参阅 [ARCHITECTURE.md](ARCHITECTURE.md)

### 模块列表

- **FirstEngine_Shader** - Shader编译和加载模块（动态库）
- **FirstEngine_Resources** - 资源管理模块（动态库）
- **FirstEngine_Renderer** - Vulkan渲染模块（动态库）
- **FirstEngine_Core** - Window和交互模块（动态库）
- **FirstEngine** - 主程序（可执行文件）

```
FirstEngine/
├── include/
│   └── FirstEngine/
│       ├── Shader/
│       ├── Resources/
│       ├── Renderer/
│       └── Core/
├── src/
│   ├── Shader/
│   │   ├── CMakeLists.txt
│   │   └── ShaderLoader.cpp
│   ├── Resources/
│   │   ├── CMakeLists.txt
│   │   └── TextureLoader.cpp
│   ├── Renderer/
│   │   ├── CMakeLists.txt
│   │   ├── VulkanRenderer.cpp
│   │   ├── Swapchain.cpp
│   │   └── Pipeline.cpp
│   ├── Core/
│   │   ├── CMakeLists.txt
│   │   └── Window.cpp
│   └── Application/
│       ├── CMakeLists.txt
│       ├── Application.cpp
│       └── main.cpp
└── CMakeLists.txt
```

## Usage

The engine provides a base `Application` class that handles window creation and the main loop. Derive from it to create your application:

```cpp
class MyApp : public FirstEngine::Core::Application {
protected:
    void OnUpdate(float deltaTime) override {
        // Your update logic
    }
    
    void OnRender() override {
        // Your render logic
    }
};
```

## Shaders

The engine requires SPIR-V compiled shaders. You need to:

1. Write GLSL shaders (vertex and fragment)
2. Compile them to SPIR-V using `glslc` (from Vulkan SDK):
   ```bash
   glslc shader.vert -o shader.vert.spv
   glslc shader.frag -o shader.frag.spv
   ```
3. Update `Pipeline.cpp` to load the shaders using `ShaderLoader::LoadShader()`

## Next Steps

- Add vertex buffer support
- Implement command buffer recording
- Add uniform buffers and descriptors
- Complete texture loading implementation
- Add camera and transform systems

## License

This project is provided as-is for educational purposes.
