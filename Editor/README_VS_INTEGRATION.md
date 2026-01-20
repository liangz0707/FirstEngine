# Visual Studio 解决方案集成

## 概述

C# 编辑器项目已集成到 CMake 生成的 Visual Studio 解决方案中。当您使用 CMake 生成 Visual Studio 解决方案时，`FirstEngineEditor` 项目会自动出现在解决方案中。

## 查看方式

1. **使用 CMake 生成解决方案**:
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

2. **打开解决方案**:
   - 打开 `build/FirstEngine.sln`
   - 在解决方案资源管理器中，您会看到 "Editor" 文件夹
   - `FirstEngineEditor` 项目位于该文件夹下

## 解决方案结构

```
FirstEngine (解决方案)
├── FirstEngine
│   ├── FirstEngine (可执行文件)
│   ├── FirstEngine_Core
│   ├── FirstEngine_Device
│   └── ...
├── Editor
│   └── FirstEngineEditor (C# 项目)
├── Tools
│   └── ...
└── ThirdPart
    └── ...
```

## 构建

### 在 Visual Studio 中构建

1. **单独构建 C# 项目**:
   - 右键点击 `FirstEngineEditor` 项目
   - 选择 "生成" 或 "重新生成"

2. **构建整个解决方案**:
   - 右键点击解决方案
   - 选择 "生成解决方案"
   - C# 项目会在 C++ 项目之后自动构建

### 构建依赖

- `FirstEngineEditor` 依赖于以下 C++ 库：
  - `FirstEngine_Core`
  - `FirstEngine_Device`
  - `FirstEngine_RHI`
  - `FirstEngine_Shader`
  - `FirstEngine_Renderer`
  - `FirstEngine_Resources`

- 构建 C# 项目时，CMake 会自动：
  1. 确保所有依赖的 C++ 库已构建
  2. 将 DLL 文件复制到 C# 项目的输出目录

## 配置

### Debug 配置

- C# 项目输出: `Editor/bin/Debug/net8.0-windows/`
- DLL 文件会自动复制到此目录

### Release 配置

- C# 项目输出: `Editor/bin/Release/net8.0-windows/`
- DLL 文件会自动复制到此目录

## 注意事项

1. **需要 .NET 8.0 SDK**: 确保已安装 .NET 8.0 SDK
2. **需要 dotnet CLI**: CMake 使用 `dotnet build` 命令构建 C# 项目
3. **DLL 复制**: DLL 文件会在构建后自动复制，无需手动操作

## 故障排除

### C# 项目不显示在解决方案中

1. 检查 `Editor/CMakeLists.txt` 是否存在
2. 检查 `Editor/FirstEngineEditor.csproj` 是否存在
3. 重新运行 CMake 配置：
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

### 构建失败

1. 检查 .NET SDK 是否安装：
   ```powershell
   dotnet --version
   ```
2. 检查项目文件路径是否正确
3. 查看 Visual Studio 输出窗口的错误信息

### DLL 未复制

1. 检查 C++ 项目是否已成功构建
2. 检查输出目录路径是否正确
3. 查看构建输出中的复制命令是否执行

## 手动构建

如果需要在 Visual Studio 外手动构建：

```powershell
cd Editor
dotnet build -c Debug
# 或
dotnet build -c Release
```
