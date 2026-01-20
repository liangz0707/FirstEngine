# 调试渲染调用问题

## 问题

C++ 代码的断点不生效，怀疑 C# 编辑器没有调用到引擎的渲染代码。

## 验证方法

### 方法 1：查看控制台输出（最简单）

我已经在 `EditorAPI.cpp` 中添加了调试输出。重新编译后运行编辑器，查看控制台输出：

**应该看到的输出**：
```
[EditorAPI] EditorAPI_PrepareFrameGraph called
[EditorAPI] EditorAPI_PrepareFrameGraph: Calling BeginFrame
[EditorAPI] EditorAPI_PrepareFrameGraph: BeginFrame completed
[EditorAPI] EditorAPI_CreateResources called
[EditorAPI] EditorAPI_Render called
[EditorAPI] EditorAPI_Render: Calling ExecuteFrameGraph
[EditorAPI] EditorAPI_Render: ExecuteFrameGraph completed
[EditorAPI] EditorAPI_SubmitFrame called
[EditorAPI] EditorAPI_SubmitFrame: Calling SubmitFrame
[EditorAPI] EditorAPI_SubmitFrame: SubmitFrame completed
```

**如果看不到这些输出**：
- 说明 C# 编辑器没有调用这些函数
- 检查 `RenderView.xaml.cs` 的 `OnCompositionTargetRendering` 是否被调用

**如果看到错误消息**：
- `Invalid engine handle` - 引擎句柄无效
- `Engine not initialized` - 引擎未初始化
- `Viewport invalid or not ready` - 视口无效或未就绪
- `No swapchain` - 没有交换链

### 方法 2：在 EditorAPI 函数中设置断点

1. **重新编译 C++ 项目**（确保使用最新的代码）

2. **在 `EditorAPI.cpp` 中设置断点**：
   - `EditorAPI_PrepareFrameGraph` 函数开头（第 333 行）
   - `EditorAPI_Render` 函数开头（第 375 行）
   - `EditorAPI_SubmitFrame` 函数开头（第 395 行）

3. **启动调试**：
   - 设置 `FirstEngineEditor` 为启动项目
   - 按 F5 启动
   - 打开编辑器窗口（确保 RenderView 面板可见）

4. **如果断点生效**：
   - 说明 C# 确实调用了 C++ 代码
   - 可以继续调试渲染流程

5. **如果断点不生效**：
   - 检查符号文件是否加载
   - 检查是否真的调用了这些函数（查看控制台输出）

### 方法 3：在 RenderContext 中设置断点

如果 EditorAPI 函数可以断到，继续在渲染代码中设置断点：

1. **`RenderContext::BeginFrame()`** - 在 `src/Renderer/RenderContext.cpp`
2. **`RenderContext::ExecuteFrameGraph()`** - 在 `src/Renderer/RenderContext.cpp`
3. **`RenderContext::SubmitFrame()`** - 在 `src/Renderer/RenderContext.cpp`

### 方法 4：检查 C# 是否调用了渲染方法

在 C# 代码中添加调试输出：

1. **打开 `RenderView.xaml.cs`**

2. **在 `OnCompositionTargetRendering` 方法中添加输出**：
   ```csharp
   private void OnCompositionTargetRendering(object? sender, EventArgs e)
   {
       System.Diagnostics.Debug.WriteLine("[C#] OnCompositionTargetRendering called");
       
       if (_renderEngine != null && _isInitialized)
       {
           System.Diagnostics.Debug.WriteLine("[C#] Calling PrepareFrameGraph");
           _renderEngine.PrepareFrameGraph();
           
           System.Diagnostics.Debug.WriteLine("[C#] Calling CreateResources");
           _renderEngine.CreateResources();
           
           System.Diagnostics.Debug.WriteLine("[C#] Calling Render");
           _renderEngine.Render();
           
           System.Diagnostics.Debug.WriteLine("[C#] Calling SubmitFrame");
           _renderEngine.SubmitFrame();
       }
   }
   ```

3. **查看 Visual Studio 输出窗口**：
   - 查看 → 输出
   - 选择 "调试" 输出
   - 应该看到 `[C#]` 开头的调试消息

## 常见问题

### Q: 看不到任何输出

**A**: 可能原因：
1. **编辑器窗口未打开** - 确保打开了包含 RenderView 的窗口
2. **视口未创建** - 检查 `CreateViewportHost` 是否成功
3. **渲染循环未启动** - 检查 `StartRenderLoop` 是否被调用

**解决**：
1. 在 `RenderView.xaml.cs` 的 `OnCompositionTargetRendering` 开头添加断点
2. 确认窗口已加载且视口已创建

### Q: 看到 "Engine not initialized"

**A**: 引擎初始化失败。

**解决**：
1. 检查 `EditorAPI_InitializeEngine` 是否成功
2. 在 `EditorAPI_InitializeEngine` 中设置断点
3. 查看初始化过程中的错误消息

### Q: 看到 "Viewport invalid or not ready"

**A**: 视口创建失败或未就绪。

**解决**：
1. 检查 `EditorAPI_CreateViewport` 是否成功
2. 检查 `EditorAPI_IsViewportReady` 返回值
3. 在 `EditorAPI_CreateViewport` 中设置断点

### Q: 断点可以设置但程序不停止

**A**: 符号文件问题或代码未执行。

**解决**：
1. 检查模块窗口中的符号状态
2. 使用 `std::cout` 输出验证代码执行
3. 确认是 Debug 配置

## 验证清单

- [ ] 重新编译了 C++ 项目（包含调试输出）
- [ ] 启动了 C# 编辑器
- [ ] 打开了包含 RenderView 的窗口
- [ ] 查看控制台输出（应该看到 `[EditorAPI]` 消息）
- [ ] 在 `EditorAPI_Render` 中设置断点
- [ ] 断点可以停止程序

## 快速测试步骤

1. **重新编译 C++ 项目**

2. **启动编辑器**：
   - 设置 `FirstEngineEditor` 为启动项目
   - 按 F5 启动

3. **打开 RenderView 面板**（如果编辑器有多个面板）

4. **查看输出**：
   - Visual Studio 输出窗口
   - 或控制台窗口
   - 应该看到 `[EditorAPI]` 开头的消息

5. **如果看到输出**：
   - 说明 C# 确实调用了 C++ 代码
   - 可以在这些函数中设置断点

6. **如果看不到输出**：
   - 检查 C# 代码是否调用了这些方法
   - 检查视口是否创建成功
