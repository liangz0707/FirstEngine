# 视口嵌入功能说明

## 功能概述

视口嵌入功能允许将 FirstEngine 的渲染窗口真正嵌入到 C# WPF 编辑器中，实现类似 UE5 的编辑器视口体验。

## 实现架构

### C++ 层 (`src/Editor/EditorAPI.cpp`)

1. **子窗口创建**:
   - 使用 Win32 API `CreateWindowEx` 创建子窗口
   - 窗口类: `"FirstEngineViewport"`
   - 窗口样式: `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN`

2. **窗口消息处理**:
   - `ViewportWindowProc`: 处理 `WM_SIZE` 等消息
   - 自动更新视口大小并重建 Swapchain

3. **API 函数**:
   - `EditorAPI_CreateViewport`: 创建视口和子窗口
   - `EditorAPI_GetViewportWindowHandle`: 获取子窗口句柄 (HWND)
   - `EditorAPI_ResizeViewport`: 调整视口大小
   - `EditorAPI_DestroyViewport`: 销毁视口

### C# 层

1. **RenderViewportHost** (`Editor/Controls/RenderViewportHost.cs`):
   - 继承自 `HwndHost`
   - 在 `BuildWindowCore` 中将子窗口重新父化到 HwndHost
   - 处理窗口大小变化

2. **RenderView** (`Editor/Views/Panels/RenderView.xaml.cs`):
   - 创建和管理 `RenderViewportHost`
   - 处理渲染循环
   - 管理视口生命周期

## 使用流程

1. **初始化引擎**:
   ```csharp
   _renderEngine.Initialize(windowHandle, width, height);
   ```
   - 创建隐藏的 GLFW 窗口用于 Vulkan 初始化
   - 初始化 Vulkan 设备

2. **创建视口**:
   ```csharp
   _renderEngine.CreateViewport(windowHandle, 0, 0, width, height);
   ```
   - C++ 创建 Win32 子窗口
   - 创建 Swapchain（当前使用主窗口的 Surface）

3. **嵌入视口**:
   ```csharp
   var viewportHandle = _renderEngine.GetViewportWindowHandle();
   _viewportHost = new RenderViewportHost { ViewportHandle = viewportHandle };
   RenderHostContainer.Children.Add(_viewportHost);
   ```
   - 获取子窗口句柄
   - 创建 HwndHost 并添加到 WPF 容器
   - HwndHost 自动将子窗口重新父化

4. **渲染循环**:
   ```csharp
   CompositionTarget.Rendering += OnCompositionTargetRendering;
   ```
   - 每帧调用 `BeginFrame`、`RenderViewport`、`EndFrame`

## 当前实现状态

### ✅ 已完成

- [x] C++ 子窗口创建
- [x] 窗口消息处理 (`WM_SIZE`)
- [x] HwndHost 集成
- [x] 窗口重新父化
- [x] 窗口大小同步
- [x] 视口生命周期管理

### ⚠️ 当前限制

1. **Swapchain 共享**:
   - 所有视口共享主窗口的 Swapchain
   - 渲染输出到主窗口，而不是子窗口
   - **影响**: 视口窗口显示，但渲染内容在主窗口

2. **渲染目标**:
   - 需要为每个视口创建独立的 Surface 和 Swapchain
   - 或实现离屏渲染到纹理，然后复制到子窗口

### 🔄 未来改进

1. **独立 Surface**:
   ```cpp
   VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
   surfaceInfo.hwnd = hwndChild;
   vkCreateWin32SurfaceKHR(instance, &surfaceInfo, ...);
   ```

2. **独立 Swapchain**:
   - 为每个视口创建独立的 Swapchain
   - 支持不同的分辨率和格式

3. **离屏渲染**:
   - 渲染到纹理
   - 将纹理复制/呈现到子窗口的 Surface

4. **输入处理**:
   - 处理鼠标和键盘输入
   - 将输入事件传递给渲染引擎

## 测试

### 验证步骤

1. **启动编辑器**:
   ```powershell
   .\bin\Debug\FirstEngine.exe --editor
   ```

2. **检查视口**:
   - 视口应该显示在编辑器中心
   - 调整窗口大小时，视口应该跟随调整
   - 视口窗口应该正确嵌入到 WPF 中

3. **检查控制台输出**:
   - 应该看到 "Engine Initialized - Viewport Embedded"
   - 不应该有窗口创建错误

### 调试技巧

1. **检查窗口句柄**:
   ```csharp
   var handle = _renderEngine.GetViewportWindowHandle();
   System.Diagnostics.Debug.WriteLine($"Viewport handle: {handle}");
   ```

2. **检查窗口可见性**:
   - 使用 Spy++ 查看窗口层次结构
   - 确认子窗口正确父化到 HwndHost

3. **检查 Vulkan 错误**:
   - 启用 Vulkan 验证层
   - 查看控制台输出中的错误信息

## 相关文件

- `src/Editor/EditorAPI.cpp`: C++ 视口实现
- `include/FirstEngine/Editor/EditorAPI.h`: C API 声明
- `Editor/Controls/RenderViewportHost.cs`: HwndHost 包装类
- `Editor/Views/Panels/RenderView.xaml.cs`: 视口视图逻辑
- `Editor/Services/RenderEngineService.cs`: P/Invoke 包装服务
- `Editor/VIEWPORT_EMBEDDING.md`: 详细技术文档
