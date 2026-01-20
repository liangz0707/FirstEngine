# 视口嵌入实现说明

## 概述

视口嵌入功能允许将 FirstEngine 的渲染窗口嵌入到 C# WPF 编辑器中，实现真正的视口集成。

## 架构

### C++ 层

1. **子窗口创建** (`EditorAPI.cpp`):
   - 使用 Win32 API `CreateWindowEx` 创建子窗口
   - 窗口类名: `"FirstEngineViewport"`
   - 窗口样式: `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN`
   - 扩展样式: `WS_EX_CLIENTEDGE`

2. **窗口消息处理**:
   - `ViewportWindowProc`: 处理 `WM_SIZE` 等消息
   - 自动更新视口大小并重建 Swapchain

3. **API 函数**:
   - `EditorAPI_CreateViewport`: 创建视口和子窗口
   - `EditorAPI_GetViewportWindowHandle`: 获取子窗口句柄 (HWND)
   - `EditorAPI_ResizeViewport`: 调整视口大小
   - `EditorAPI_DestroyViewport`: 销毁视口和子窗口

### C# 层

1. **RenderViewportHost** (`Editor/Controls/RenderViewportHost.cs`):
   - 继承自 `HwndHost`
   - 在 `BuildWindowCore` 中将子窗口重新父化到 HwndHost
   - 处理窗口大小变化 (`OnRenderSizeChanged`)

2. **RenderView** (`Editor/Views/Panels/RenderView.xaml.cs`):
   - 创建 `RenderViewportHost` 实例
   - 将视口窗口句柄传递给 `RenderViewportHost`
   - 管理渲染循环和视口生命周期

## 工作流程

```
1. C# 编辑器启动
   └─> RenderView.Loaded
       └─> InitializeRenderEngine()
           └─> RenderEngineService.Initialize()
               └─> EditorAPI_InitializeEngine()
                   └─> 创建隐藏的 GLFW 窗口（用于 Vulkan 初始化）

2. 创建视口
   └─> CreateViewportHost()
       └─> RenderEngineService.CreateViewport()
           └─> EditorAPI_CreateViewport()
               ├─> 创建 Win32 子窗口 (CreateWindowEx)
               ├─> 注册窗口类 (RegisterClassEx)
               ├─> 设置窗口过程 (ViewportWindowProc)
               └─> 创建 Swapchain

3. 嵌入视口
   └─> RenderViewportHost.BuildWindowCore()
       └─> ReparentViewportWindow()
           └─> SetParent(viewportHandle, hwndHostHandle)
               └─> 子窗口现在嵌入到 WPF 中

4. 渲染循环
   └─> CompositionTarget.Rendering
       └─> RenderEngineService.BeginFrame()
       └─> RenderEngineService.RenderViewport()
       └─> RenderEngineService.EndFrame()
```

## 关键实现细节

### 1. 窗口初始化

**问题**: `VulkanDevice::Initialize` 需要 `GLFWwindow*`，但编辑器传入的是 `HWND`

**解决方案**: 
- 创建一个隐藏的 GLFW 窗口用于 Vulkan 初始化
- 子窗口使用 Win32 API 直接创建
- Swapchain 使用主窗口的 Surface（当前限制）

### 2. 窗口重新父化

**过程**:
1. C++ 创建子窗口，父窗口是 WPF 主窗口
2. C# 获取子窗口句柄
3. `HwndHost.BuildWindowCore` 被调用
4. 将子窗口重新父化到 `HwndHost` 的窗口

**代码**:
```cpp
// C++: 创建子窗口
HWND hwndChild = CreateWindowEx(..., hwndParent, ...);

// C#: 重新父化
SetParent(viewportHandle, hwndHostHandle);
```

### 3. 窗口大小同步

**机制**:
- WPF 大小变化 → `OnRenderHostSizeChanged`
- 调用 `EditorAPI_ResizeViewport`
- C++ 更新子窗口大小 (`SetWindowPos`)
- Swapchain 重建 (`swapchain->Recreate()`)

### 4. 窗口消息处理

**窗口过程** (`ViewportWindowProc`):
- `WM_SIZE`: 更新视口大小，重建 Swapchain
- `WM_DESTROY`: 清理资源
- 其他消息: 转发给 `DefWindowProc`

## 当前限制

1. **Swapchain 共享**: 
   - 当前所有视口共享主窗口的 Swapchain
   - 未来需要为每个视口创建独立的 Surface 和 Swapchain

2. **渲染目标**:
   - 渲染输出到主窗口，而不是子窗口
   - 需要实现离屏渲染或纹理渲染到子窗口

3. **多视口支持**:
   - 当前实现支持单个视口
   - 多视口需要额外的资源管理

## 未来改进

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
   - 将纹理复制到子窗口的 Surface

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

3. **检查控制台输出**:
   - 应该看到 "Engine Initialized - Viewport Embedded"
   - 不应该有窗口创建错误

### 常见问题

1. **视口不显示**:
   - 检查 DLL 是否正确加载
   - 检查窗口句柄是否有效
   - 查看控制台错误信息

2. **窗口大小不正确**:
   - 检查 `OnRenderHostSizeChanged` 是否被调用
   - 检查 `SetWindowPos` 是否成功

3. **渲染不显示**:
   - 检查 Swapchain 是否创建成功
   - 检查渲染循环是否运行
   - 检查 Vulkan 验证层错误

## 相关文件

- `src/Editor/EditorAPI.cpp`: C++ 视口实现
- `include/FirstEngine/Editor/EditorAPI.h`: C API 声明
- `Editor/Controls/RenderViewportHost.cs`: HwndHost 包装
- `Editor/Views/Panels/RenderView.xaml.cs`: 视口视图逻辑
- `Editor/Services/RenderEngineService.cs`: P/Invoke 包装
