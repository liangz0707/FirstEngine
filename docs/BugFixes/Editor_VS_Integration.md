# C# 编辑器项目 Visual Studio 集成指南

## 概述

C# 编辑器项目 (`FirstEngineEditor`) 已集成到 CMake 生成的 Visual Studio 解决方案中。当您使用 CMake 生成 Visual Studio 解决方案时，编辑器项目会自动出现在解决方案的 "Editor" 文件夹下。

## 使用方法

### 1. 生成 Visual Studio 解决方案

```powershell
# 在项目根目录
cmake -B build -G "Visual Studio 17 2022" -A x64
```

### 2. 打开解决方案

打开生成的 `build/FirstEngine.sln` 文件，您将在解决方案资源管理器中看到：

```
FirstEngine (解决方案)
├── FirstEngine
│   ├── FirstEngine (可执行文件)
│   ├── FirstEngine_Core
│   ├── FirstEngine_Device
│   └── ...
├── Editor                    ← 新增的文件夹
│   └── FirstEngineEditor     ← C# 编辑器项目
├── Tools
│   └── ...
└── ThirdPart
    └── ...
```

### 3. 构建项目

#### 方式一：在 Visual Studio 中构建

1. **单独构建 C# 项目**:
   - 右键点击 `FirstEngineEditor` 项目
   - 选择 "生成" 或 "重新生成"

2. **构建整个解决方案**:
   - 右键点击解决方案
   - 选择 "生成解决方案"
   - C# 项目会在所有 C++ 依赖项目构建完成后自动构建

#### 方式二：使用命令行

```powershell
# 构建整个解决方案
cmake --build build --config Debug

# 或只构建 C# 项目
cmake --build build --target FirstEngineEditor --config Debug
```

## 工作原理

### CMake 集成

1. **CMakeLists.txt 位置**: `Editor/CMakeLists.txt`
2. **添加方式**: 在根 `CMakeLists.txt` 中通过 `add_subdirectory(Editor)` 添加
3. **目标类型**: 使用 `add_custom_target` 创建自定义构建目标

### 构建流程

1. **依赖检查**: C# 项目依赖于所有 C++ 库
2. **构建顺序**: 
   - 首先构建所有 C++ 库
   - 然后构建 C# 项目
3. **DLL 复制**: 构建完成后自动将 DLL 复制到 C# 项目输出目录

### 输出目录

- **Debug**: `Editor/bin/Debug/net8.0-windows/`
- **Release**: `Editor/bin/Release/net8.0-windows/`

DLL 文件会自动复制到对应配置的输出目录。

## 配置要求

### 必需组件

1. **.NET 8.0 SDK**: 
   ```powershell
   dotnet --version
   # 应该显示 8.x.x 或更高版本
   ```

2. **Visual Studio 2022**: 推荐使用 Visual Studio 2022 或更高版本

3. **CMake 3.20+**: 
   ```powershell
   cmake --version
   ```

### 环境变量

确保 `dotnet` 命令在 PATH 中可用：

```powershell
where dotnet
# 应该显示 dotnet.exe 的路径
```

## 故障排除

### 问题 1: C# 项目不显示在解决方案中

**原因**: CMake 配置时未找到项目文件或路径错误

**解决方法**:
1. 检查 `Editor/FirstEngineEditor.csproj` 是否存在
2. 检查 `Editor/CMakeLists.txt` 是否存在
3. 重新运行 CMake 配置：
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

### 问题 2: 构建失败 - dotnet 未找到

**原因**: `dotnet` 命令不在 PATH 中

**解决方法**:
1. 安装 .NET 8.0 SDK
2. 重启 Visual Studio 或命令行窗口
3. 验证 dotnet 可用：
   ```powershell
   dotnet --version
   ```

### 问题 3: DLL 未复制到输出目录

**原因**: C++ 项目未构建或路径错误

**解决方法**:
1. 确保所有 C++ 项目已成功构建
2. 检查输出目录路径是否正确
3. 查看构建输出中的复制命令是否执行

### 问题 4: 项目显示但无法构建

**原因**: 可能是自定义目标的限制

**解决方法**:
1. 在 Visual Studio 中右键项目 → "生成"
2. 查看输出窗口的错误信息
3. 尝试在命令行手动构建：
   ```powershell
   cd Editor
   dotnet build -c Debug
   ```

## 高级配置

### 修改构建配置

编辑 `Editor/CMakeLists.txt` 可以修改：

- **构建命令**: 修改 `dotnet build` 的参数
- **输出目录**: 修改 `EDITOR_OUTPUT_DIR`
- **依赖关系**: 修改 `add_dependencies` 列表

### 添加预构建步骤

在 `Editor/CMakeLists.txt` 中添加：

```cmake
add_custom_command(TARGET FirstEngineEditor PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Pre-build step..."
    COMMENT "Running pre-build steps"
)
```

### 添加后构建步骤

在 `Editor/CMakeLists.txt` 中添加：

```cmake
add_custom_command(TARGET FirstEngineEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Post-build step..."
    COMMENT "Running post-build steps"
)
```

## 注意事项

1. **项目类型**: C# 项目在 Visual Studio 中显示为 "自定义目标"，不是原生 C# 项目类型
2. **IntelliSense**: 可能无法获得完整的 C# IntelliSense 支持
3. **调试**: 需要在 Visual Studio 中单独打开 `.csproj` 文件进行调试
4. **文件编辑**: 可以直接在 Visual Studio 中编辑 C# 文件，但构建需要通过 CMake 目标

## 最佳实践

1. **开发流程**:
   - 在 Visual Studio 中编辑 C# 代码
   - 使用 CMake 目标构建项目
   - 或使用 `dotnet build` 直接构建

2. **调试**:
   - 在 Visual Studio 中打开 `Editor/FirstEngineEditor.csproj`
   - 设置启动项目
   - 按 F5 开始调试

3. **构建优化**:
   - 使用 `--no-incremental` 标志确保完整构建
   - 定期清理构建目录

## 相关文件

- `Editor/CMakeLists.txt`: CMake 集成配置
- `Editor/FirstEngineEditor.csproj`: C# 项目文件
- `CMakeLists.txt`: 根 CMake 配置文件（包含 `add_subdirectory(Editor)`）
- `Editor/README_VS_INTEGRATION.md`: 详细的集成说明
