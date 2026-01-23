# RenderDoc 截帧支持设置指南

## 概述

RenderDoc 是一个强大的图形调试工具，可以捕获和分析 Vulkan 渲染调用。本指南说明如何在 FirstEngine 中启用和使用 RenderDoc。

## 问题说明

在调试模式下（`_DEBUG`），RenderDoc 默认被禁用，以避免与调试器冲突。这导致编辑器启动时无法截帧。

## 解决方案

### 方法 1：使用环境变量（推荐）

设置环境变量 `FIRSTENGINE_ENABLE_RENDERDOC=1` 来强制启用 RenderDoc，即使在调试模式下也能使用。

#### Windows PowerShell
```powershell
$env:FIRSTENGINE_ENABLE_RENDERDOC = "1"
```

#### Windows CMD
```cmd
set FIRSTENGINE_ENABLE_RENDERDOC=1
```

#### 在 Visual Studio 中设置
1. 右键点击项目 → Properties
2. Configuration Properties → Debugging
3. Environment: 添加 `FIRSTENGINE_ENABLE_RENDERDOC=1`

#### 在 C# 编辑器中设置
在 `Editor/Views/MainWindow.xaml.cs` 或启动代码中设置环境变量：
```csharp
Environment.SetEnvironmentVariable("FIRSTENGINE_ENABLE_RENDERDOC", "1");
```

### 方法 2：从 RenderDoc UI 启动

1. 打开 RenderDoc UI
2. 点击 "Launch Application"
3. 选择 `FirstEngineEditor.exe` 或你的可执行文件
4. 点击 "Launch"

这样 RenderDoc 会自动注入到进程中，无需设置环境变量。

### 方法 3：注入到运行中的进程

1. 打开 RenderDoc UI
2. 点击 "Inject into Process"
3. 选择正在运行的 `FirstEngineEditor.exe` 进程
4. 点击 "Inject"

## 验证 RenderDoc 是否启用

运行应用程序后，检查控制台输出：

### 成功启用
```
RenderDoc: Force enabled via FIRSTENGINE_ENABLE_RENDERDOC environment variable
RenderDoc: Loaded from: C:\Program Files\RenderDoc\renderdoc.dll
RenderDoc: API initialized successfully!
RenderDoc: Target control is connected!
```

### 未启用（调试模式）
```
RenderDoc: Skipping initialization in debug mode (set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable)
```

### RenderDoc 未安装
```
RenderDoc: DLL not found. RenderDoc capture will not be available.
RenderDoc: To use RenderDoc:
RenderDoc:   1. Install RenderDoc from https://renderdoc.org/
RenderDoc:   2. Launch this application from RenderDoc UI (Launch Application)
RenderDoc:   3. Or inject RenderDoc into this process (Inject into Process)
```

## 使用 RenderDoc 截帧

### 自动截帧

当 RenderDoc 启用后，每帧都会自动调用 `StartFrameCapture` 和 `EndFrameCapture`。你可以在 RenderDoc UI 中：

1. 点击 "Trigger Capture" 按钮来捕获当前帧
2. 或者使用快捷键（默认 F12）

### 手动截帧

如果需要手动控制截帧，可以修改代码：

```cpp
// 在需要截帧的地方
if (FirstEngine::Core::RenderDocHelper::IsAvailable()) {
    FirstEngine::Core::RenderDocHelper::BeginFrame();
    // ... 渲染代码 ...
    FirstEngine::Core::RenderDocHelper::EndFrame();
}
```

## 代码修改说明

### 修改的文件

1. **include/FirstEngine/Core/RenderDoc.h**
   - 添加了环境变量 `FIRSTENGINE_ENABLE_RENDERDOC` 支持
   - 在调试模式下，如果设置了环境变量，仍然可以启用 RenderDoc

2. **src/Core/RenderDoc.cpp**
   - 同步更新了环境变量检查逻辑
   - 确保头文件和实现文件的一致性

### 关键改动

#### 之前
```cpp
#ifdef _DEBUG
std::cout << "RenderDoc: Skipping initialization in debug mode" << std::endl;
return;
#endif
```

#### 之后
```cpp
bool forceEnable = false;
char* envValue = nullptr;
size_t envSize = 0;
if (_dupenv_s(&envValue, &envSize, "FIRSTENGINE_ENABLE_RENDERDOC") == 0 && envValue) {
    forceEnable = (std::string(envValue) == "1" || std::string(envValue) == "true");
    free(envValue);
}

#ifdef _DEBUG
if (!forceEnable) {
    std::cout << "RenderDoc: Skipping initialization in debug mode (set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable)" << std::endl;
    return;
} else {
    std::cout << "RenderDoc: Force enabled via FIRSTENGINE_ENABLE_RENDERDOC environment variable" << std::endl;
}
#endif
```

## 常见问题

### Q: 为什么在调试模式下 RenderDoc 默认被禁用？

A: 为了避免与 Visual Studio 调试器冲突，以及防止在调试时出现访问违规错误。RenderDoc 和调试器可能会同时尝试拦截 Vulkan 调用，导致冲突。

### Q: 设置环境变量后仍然无法截帧？

A: 请检查：
1. 环境变量是否正确设置（`FIRSTENGINE_ENABLE_RENDERDOC=1`）
2. RenderDoc UI 是否正在运行
3. RenderDoc 是否正确安装（检查 `C:\Program Files\RenderDoc\renderdoc.dll` 是否存在）
4. 控制台输出是否显示 "RenderDoc: API initialized successfully!"

### Q: 如何确认 RenderDoc 正在工作？

A: 查看控制台输出：
- 如果看到 "RenderDoc: Target control is connected!"，说明 RenderDoc 已连接
- 如果看到 "RenderDoc: Target control is NOT connected"，需要确保 RenderDoc UI 正在运行

### Q: 可以在 Release 模式下使用 RenderDoc 吗？

A: 可以。在 Release 模式下，RenderDoc 默认启用（不需要设置环境变量），只要 RenderDoc UI 正在运行即可。

## 最佳实践

1. **开发时**：设置环境变量 `FIRSTENGINE_ENABLE_RENDERDOC=1`，以便在调试时也能使用 RenderDoc
2. **性能分析**：使用 Release 模式，RenderDoc 会自动启用
3. **调试渲染问题**：从 RenderDoc UI 启动应用程序，这样可以确保 RenderDoc 正确注入

## 相关文档

- RenderDoc 官方网站: https://renderdoc.org/
- RenderDoc 文档: https://renderdoc.org/docs/
