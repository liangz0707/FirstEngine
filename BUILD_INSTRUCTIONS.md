# FirstEngine 编译说明

## 快速编译

### Windows (PowerShell)

```powershell
# 方法 1: 使用编译脚本（推荐）
.\build_and_fix.ps1

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

## 已修复的问题

1. **Application CMakeLists.txt**: 移除了重复的 `project()` 定义，使用直接的目标名称 `FirstEngine`
2. **RHI CMakeLists.txt**: 添加了 `RHI.cpp` 源文件

## 常见错误及解决方法

### 错误 1: CMake 找不到 Vulkan

**错误信息**: `Could not find a package configuration file provided by "Vulkan"`

**解决方法**:
- 确保已安装 Vulkan SDK
- 或将 Vulkan SDK 复制到 `external/vulkan` 目录

### 错误 2: CMake 找不到 GLFW

**错误信息**: `Could not find glfw3`

**解决方法**:
- GLFW 会自动从 `third_party/glfw` 编译
- 确保已初始化 Git 子模块: `git submodule update --init --recursive`

### 错误 3: 编译错误

如果遇到编译错误，请检查：
1. 所有源文件是否存在
2. 所有头文件路径是否正确
3. 所有依赖库是否已正确链接

## 编译输出

编译成功后，可执行文件位置：
- **Windows**: `build/bin/Release/FirstEngine.exe`
- **Linux/Mac**: `build/bin/FirstEngine`

DLL 文件位置（Windows）:
- `build/lib/Release/FirstEngine_*.dll`

## 验证编译

运行编译后的程序：

```bash
# Windows
.\build\bin\Release\FirstEngine.exe

# Linux/Mac
./build/bin/FirstEngine
```

如果程序正常启动并显示窗口，说明编译成功！
