# FirstEngine 编译指南

## 快速编译

### Windows (PowerShell)

```powershell
# 方法 1: 使用编译脚本
.\build_project.ps1

# 方法 2: 手动编译
mkdir build -Force
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/Mac

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 常见问题

### 1. CMake 找不到 Vulkan

**错误信息**: `Could not find a package configuration file provided by "Vulkan"`

**解决方法**:
- 确保已安装 Vulkan SDK
- 设置环境变量 `VULKAN_SDK`
- 或者将 Vulkan SDK 复制到 `external/vulkan` 目录

### 2. CMake 找不到 GLFW

**错误信息**: `Could not find glfw3`

**解决方法**:
- GLFW 应该自动从 `third_party/glfw` 编译
- 确保已初始化 Git 子模块: `git submodule update --init --recursive`

### 3. 缺少源文件错误

**错误信息**: `No rule to make target 'src/XXX/XXX.cpp'`

**解决方法**:
- 检查 `src/` 目录下是否有对应的源文件
- 检查 `CMakeLists.txt` 中的源文件列表是否正确

### 4. 链接错误

**错误信息**: `unresolved external symbol`

**解决方法**:
- 确保所有模块的 DLL 都正确链接
- 检查 `target_link_libraries` 配置
- 在 Windows 上，确保 DLL 在 PATH 中或与可执行文件在同一目录

## 依赖检查

### 必需的依赖

1. **CMake** (>= 3.20)
2. **C++ 编译器** (支持 C++17)
   - Windows: Visual Studio 2019 或更新版本
   - Linux: GCC 7+ 或 Clang 5+
3. **Vulkan SDK**
4. **Python 3.x** (可选，用于 Python 模块)

### Git 子模块

确保所有子模块都已初始化：

```bash
git submodule update --init --recursive
```

子模块包括：
- `third_party/glfw`
- `third_party/glm`
- `third_party/SPIRV-Cross`
- `third_party/glslang`
- 等等

## 编译选项

### Release 版本

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Debug 版本

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### 指定生成器 (Windows)

```bash
# Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64

# Visual Studio 2019
cmake .. -G "Visual Studio 16 2019" -A x64

# MinGW
cmake .. -G "MinGW Makefiles"
```

## 输出目录

编译后的文件位置：

- **可执行文件**: `build/bin/Release/FirstEngine.exe` (Windows) 或 `build/bin/FirstEngine` (Linux)
- **DLL 文件**: `build/lib/Release/` (Windows) 或 `build/lib/` (Linux)
- **配置文件**: `configs/deferred_rendering.pipeline`

## 验证编译

编译成功后，运行：

```bash
# Windows
.\build\bin\Release\FirstEngine.exe

# Linux
./build/bin/FirstEngine
```

如果程序正常启动并显示窗口，说明编译成功！
