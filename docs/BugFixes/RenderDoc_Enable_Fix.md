# RenderDoc 截帧支持修复

## 日期
2024年（当前会话）

## 问题概述

编辑器启动时无法使用 RenderDoc 截帧，因为在调试模式（`_DEBUG`）下 RenderDoc 默认被禁用。

## 问题原因

RenderDoc 的初始化代码在以下情况下会被跳过：
1. 编译为 Debug 模式（`#ifdef _DEBUG`）
2. 检测到调试器附加（`IsDebuggerPresent()`）

这是为了避免与 Visual Studio 调试器冲突，但导致在开发时无法使用 RenderDoc。

## 解决方案

### 1. 添加环境变量支持

添加了环境变量 `FIRSTENGINE_ENABLE_RENDERDOC`，允许在调试模式下强制启用 RenderDoc。

**环境变量值**：
- `1` 或 `true`：强制启用 RenderDoc（即使在调试模式下）
- 未设置或其他值：使用默认行为（调试模式下禁用）

### 2. 在 C# 编辑器中自动启用

修改了 `Editor/App.xaml.cs`，在应用程序启动时自动设置环境变量，确保 RenderDoc 始终可用。

## 代码修改说明

### 修改的文件

1. **include/FirstEngine/Core/RenderDoc.h**
   - 添加了环境变量检查逻辑
   - 在 `Initialize()`、`BeginFrame()`、`EndFrame()`、`IsAvailable()` 中都添加了环境变量支持
   - 添加了 `#include <stdlib.h>` 和 `#include <string>` 用于环境变量读取

2. **src/Core/RenderDoc.cpp**
   - 同步更新了环境变量检查逻辑
   - 确保与头文件实现一致

3. **Editor/App.xaml.cs**
   - 在 `OnStartup()` 中自动设置 `FIRSTENGINE_ENABLE_RENDERDOC=1`
   - 添加了 `using System;` 用于 `Environment.SetEnvironmentVariable`

### 关键代码改动

#### 环境变量检查逻辑
```cpp
// Check environment variable to force enable RenderDoc even in debug mode
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

#### C# 编辑器自动启用
```csharp
// Enable RenderDoc support for frame capture
// This allows RenderDoc to work even in debug mode
Environment.SetEnvironmentVariable("FIRSTENGINE_ENABLE_RENDERDOC", "1");
Console.WriteLine("RenderDoc support enabled (set FIRSTENGINE_ENABLE_RENDERDOC=1)");
```

## 使用方法

### 方法 1：自动启用（推荐）

C# 编辑器现在会自动启用 RenderDoc 支持，无需任何配置。只需：
1. 确保 RenderDoc UI 正在运行
2. 启动编辑器
3. 在 RenderDoc UI 中点击 "Trigger Capture" 或按 F12 截帧

### 方法 2：手动设置环境变量

如果需要在其他应用程序中启用，可以手动设置环境变量：

#### Windows PowerShell
```powershell
$env:FIRSTENGINE_ENABLE_RENDERDOC = "1"
```

#### Windows CMD
```cmd
set FIRSTENGINE_ENABLE_RENDERDOC=1
```

#### Visual Studio 项目属性
1. 右键点击项目 → Properties
2. Configuration Properties → Debugging
3. Environment: 添加 `FIRSTENGINE_ENABLE_RENDERDOC=1`

### 方法 3：从 RenderDoc UI 启动

1. 打开 RenderDoc UI
2. 点击 "Launch Application"
3. 选择可执行文件
4. 点击 "Launch"

这样 RenderDoc 会自动注入，无需设置环境变量。

## 验证 RenderDoc 是否启用

运行应用程序后，检查控制台输出：

### ✅ 成功启用
```
RenderDoc support enabled (set FIRSTENGINE_ENABLE_RENDERDOC=1)
RenderDoc: Force enabled via FIRSTENGINE_ENABLE_RENDERDOC environment variable
RenderDoc: Loaded from: C:\Program Files\RenderDoc\renderdoc.dll
RenderDoc: API initialized successfully!
RenderDoc: Target control is connected!
```

### ❌ 未启用（未设置环境变量）
```
RenderDoc: Skipping initialization in debug mode (set FIRSTENGINE_ENABLE_RENDERDOC=1 to enable)
```

### ❌ RenderDoc 未安装
```
RenderDoc: DLL not found. RenderDoc capture will not be available.
```

## 注意事项

1. **RenderDoc UI 必须运行**：即使设置了环境变量，也需要确保 RenderDoc UI 正在运行
2. **调试器冲突**：在某些情况下，RenderDoc 和调试器可能仍然会冲突。如果遇到问题，尝试：
   - 从 RenderDoc UI 启动应用程序（而不是从 Visual Studio）
   - 或者在不附加调试器的情况下运行
3. **性能影响**：RenderDoc 会略微影响性能，建议在需要调试时才启用

## 相关文档

- RenderDoc 设置指南: `docs/RenderDoc_Setup_Guide.md`
- RenderDoc 官方网站: https://renderdoc.org/
