# Visual Studio 解决方案结构说明

本文档说明FirstEngine项目在Visual Studio中的解决方案文件夹组织结构。

## 解决方案文件夹结构

### FirstEngine（核心模块）
包含FirstEngine引擎自己的核心模块：

- **FirstEngine** - 主应用程序（可执行文件）
- **FirstEngine_Core** - 窗口管理和主循环模块
- **FirstEngine_Device** - Vulkan图形API封装模块
- **FirstEngine_Shader** - Shader编译和加载模块
- **FirstEngine_Resources** - 资源管理模块
- **FirstEngine_Python** - Python脚本支持模块

### Tools（工具项目）
包含开发工具和命令行工具：

- **ShaderManager** - Shader编译和转换命令行工具

### ThirdPart（第三方库）
包含所有第三方依赖库，按库分组：

#### ThirdPart/SPIRV-Cross
- spirv-cross-core
- spirv-cross-glsl
- spirv-cross-hlsl
- spirv-cross-msl
- spirv-cross-reflect
- spirv-cross-util
- spirv-cross-cpp
- spirv-cross-c
- spirv-cross

#### ThirdPart/glslang
- glslang
- SPIRV
- GenericCodeGen
- MachineIndependent
- OSDependent
- glslang-default-resource-limits

#### ThirdPart/Assimp
- assimp
- zlibstatic
- UpdateAssimpLibsDebugSymbolsAndDLLs

#### ThirdPart/GLFW
- glfw

#### ThirdPart/GLM
- glm

#### ThirdPart/pybind11
- pybind11相关目标

## 如何查看

在Visual Studio中打开 `build/FirstEngine.sln`，您将看到：

```
FirstEngine (解决方案)
├── FirstEngine
│   ├── FirstEngine
│   ├── FirstEngine_Core
│   ├── FirstEngine_Device
│   ├── FirstEngine_Shader
│   ├── FirstEngine_Resources
│   └── FirstEngine_Python
├── Tools
│   └── ShaderManager
└── ThirdPart
    ├── SPIRV-Cross
    │   └── (多个SPIRV-Cross项目)
    ├── glslang
    │   └── (多个glslang项目)
    ├── Assimp
    │   └── (Assimp相关项目)
    ├── GLFW
    │   └── glfw
    ├── GLM
    │   └── glm
    └── pybind11
        └── (pybind11相关项目)
```

## 优势

1. **清晰的组织结构** - 核心模块、工具和第三方库分离
2. **易于管理** - 可以折叠第三方库文件夹，专注于核心开发
3. **减少混乱** - 第三方库项目不会与自己的项目混在一起
4. **便于维护** - 清楚地知道哪些是核心模块，哪些是依赖

## 技术实现

使用CMake的`FOLDER`属性来组织Visual Studio解决方案文件夹：

```cmake
set_target_properties(TargetName PROPERTIES FOLDER "FolderName")
```

这会在Visual Studio中创建逻辑文件夹，但不会改变实际的文件系统结构。
