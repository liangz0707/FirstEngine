# FirstEngine 模块化架构

## 模块结构

项目已重构为独立的动态链接库模块，每个模块可以独立开发和编译。

### 模块列表

1. **FirstEngine_Shader** - Shader编译和加载模块
   - 位置: `src/Shader/`
   - 功能: Shader文件加载、SPIR-V编译
   - 导出符号: `FE_SHADER_API`

2. **FirstEngine_Resources** - 资源管理模块
   - 位置: `src/Resources/`
   - 功能: 纹理、模型、场景等资源加载
   - 导出符号: `FE_RESOURCES_API`

3. **FirstEngine_Renderer** - Vulkan渲染模块
   - 位置: `src/Renderer/`
   - 功能: Vulkan初始化、Swapchain、Pipeline
   - 依赖: FirstEngine_Shader, FirstEngine_Resources
   - 导出符号: `FE_RENDERER_API`

4. **FirstEngine_Core** - Window和交互模块
   - 位置: `src/Core/`
   - 功能: GLFW窗口管理、输入处理
   - 导出符号: `FE_CORE_API`

5. **FirstEngine** - 主程序
   - 位置: `src/Application/`
   - 功能: 主循环、Application类
   - 链接: 所有上述模块

## 目录结构

```
FirstEngine/
├── include/
│   └── FirstEngine/
│       ├── Shader/
│       │   ├── Export.h
│       │   └── ShaderLoader.h
│       ├── Resources/
│       │   ├── Export.h
│       │   └── TextureLoader.h
│       ├── Renderer/
│       │   ├── Export.h
│       │   ├── VulkanRenderer.h
│       │   ├── Swapchain.h
│       │   └── Pipeline.h
│       └── Core/
│           ├── Export.h
│           ├── Window.h
│           └── Application.h
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

## 独立编译

每个模块都有独立的 `CMakeLists.txt`，可以单独编译：

```bash
# 编译所有模块
cd build
cmake ..
cmake --build .

# 只编译Shader模块
cmake --build . --target FirstEngine_Shader

# 只编译Renderer模块（会自动编译依赖）
cmake --build . --target FirstEngine_Renderer
```

## 导出符号

所有公开的类和函数都需要使用相应的导出宏：

- `FE_SHADER_API` - Shader模块
- `FE_RESOURCES_API` - Resources模块
- `FE_RENDERER_API` - Renderer模块
- `FE_CORE_API` - Core模块

示例：
```cpp
#include "FirstEngine/Shader/Export.h"

namespace FirstEngine {
    namespace Shader {
        class FE_SHADER_API ShaderLoader {
            // ...
        };
    }
}
```

## 依赖关系

```
FirstEngine (Application)
    ├── FirstEngine_Core
    ├── FirstEngine_Renderer
    │   ├── FirstEngine_Shader
    │   └── FirstEngine_Resources
    ├── FirstEngine_Shader
    └── FirstEngine_Resources
```
