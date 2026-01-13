# FirstEngine 模块说明

## 核心模块

### FirstEngine_Core
窗口管理和主循环模块。

### FirstEngine_Device
Vulkan图形API封装模块，包含所有Vulkan相关代码。

### FirstEngine_Shader
Shader编译和加载模块，图形API无关。

### FirstEngine_Resources
资源管理模块，支持多种格式的资源加载。

### FirstEngine_Python
Python脚本支持模块，实现C++和Python交互。

### FirstEngine_Application
主应用程序入口。

### ShaderManager
命令行工具，用于Shader编译和转换。

## 资源模块详细说明

### 图像加载功能
- **支持的格式**: JPEG, PNG, BMP, TGA, DDS, TIFF, HDR
- **库**: stb_image (单文件头文件库)
- **特性**:
  - 从文件或内存加载
  - 自动格式检测
  - 支持多种颜色通道

### 模型加载功能
- **支持的格式**: FBX, OBJ, DAE, 3DS, BLEND等40+种格式
- **库**: Assimp (Open Asset Import Library)
- **特性**:
  - 网格加载（顶点、法线、纹理坐标）
  - 材质信息
  - 骨骼/骨架数据
  - 多网格支持

## Python模块详细说明

### 功能
- 嵌入Python解释器
- 执行Python脚本和代码字符串
- 错误处理和错误信息获取

### 要求
- Python 3.x (Python 2.7不支持)
- pybind11 (已包含为子模块)

### 使用示例
```cpp
#include "FirstEngine/Python/PythonEngine.h"

FirstEngine::Python::PythonEngine engine;
engine.Initialize();
engine.ExecuteString("print('Hello from Python!')");
engine.ExecuteFile("script.py");
```

## 第三方依赖

所有第三方库都作为Git子模块集成在`third_party/`目录中：

- **SPIRV-Cross**: SPIR-V转换器
- **glslang**: GLSL/HLSL编译器
- **GLFW**: 窗口管理
- **GLM**: 数学库
- **pybind11**: Python绑定
- **stb**: 图像加载
- **assimp**: 模型加载

## 构建说明

1. 初始化子模块：
```bash
git submodule update --init --recursive
```

2. 配置CMake：
```bash
mkdir build
cd build
cmake ..
```

3. 编译：
```bash
cmake --build . --config Release
```

**注意**: Python模块需要Python 3.x。如果系统未安装Python 3，该模块将被跳过。
