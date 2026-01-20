# 编辑器渲染问题修复

## 问题描述

编辑器启动后，中间的渲染窗口显示为纯黑色，没有进行渲染。

## 根本原因

1. **EditorAPI_RenderViewport 函数是空的**：只有占位符注释，没有实际的渲染逻辑
2. **缺少渲染资源**：RenderEngine 结构中没有命令缓冲区、同步对象等
3. **场景未加载**：编辑器没有加载场景数据
4. **渲染循环未自动启动**：需要手动点击按钮才能启动

## 修复内容

### 1. 扩展 RenderEngine 结构

添加了必要的渲染资源：
```cpp
struct RenderEngine {
    // ... 现有字段 ...
    
    // 新增渲染资源
    FirstEngine::Renderer::FrameGraphExecutionPlan frameGraphPlan;
    FirstEngine::Renderer::RenderCommandList frameGraphRenderCommands;
    FirstEngine::Renderer::CommandRecorder commandRecorder;
    FirstEngine::Resources::Scene* scene = nullptr;
    std::unique_ptr<FirstEngine::RHI::ICommandBuffer> commandBuffer;
    FirstEngine::RHI::FenceHandle inFlightFence = nullptr;
    FirstEngine::RHI::SemaphoreHandle imageAvailableSemaphore = nullptr;
    FirstEngine::RHI::SemaphoreHandle renderFinishedSemaphore = nullptr;
    uint32_t currentImageIndex = 0;
};
```

### 2. 实现 EditorAPI_RenderViewport

完整的渲染流程：
1. 等待上一帧完成（WaitForFence）
2. 获取下一帧的 swapchain 图像（AcquireNextImage）
3. 创建命令缓冲区
4. 转换图像布局（Undefined → Color Attachment）
5. 记录 FrameGraph 渲染命令
6. 转换图像布局（Color Attachment → Present）
7. 提交命令缓冲区
8. 呈现图像（Present）

### 3. 实现 EditorAPI_BeginFrame

完整的 FrameGraph 构建流程：
1. 释放上一帧的资源
2. 清除 FrameGraph
3. 从 Pipeline 重建 FrameGraph
4. 构建执行计划
5. 执行 FrameGraph 生成渲染命令
6. 处理计划中的资源

### 4. 初始化场景和资源

- 初始化 ResourceManager
- 添加 Package 目录搜索路径
- 创建默认场景
- 尝试加载 example_scene.json

### 5. 自动启动渲染循环

在视口创建成功后自动启动渲染循环，无需手动点击按钮。

## 已知限制

### Swapchain 表面问题

**当前状态**：
- `CreateSwapchain` 忽略 `windowHandle` 参数
- 使用隐藏 GLFW 窗口的表面（来自 VulkanRenderer）
- 渲染输出到隐藏窗口，而不是视口窗口

**影响**：
- 视口窗口显示为黑色（因为没有渲染到它的表面）
- 实际渲染发生在隐藏窗口（不可见）

**解决方案（待实现）**：
1. 修改 `VulkanDevice::CreateSwapchain` 支持从 HWND 创建表面
2. 或修改 `VulkanRenderer` 支持多个表面
3. 为视口窗口创建独立的表面和 swapchain

## 测试建议

1. **检查控制台输出**：
   - 查看是否有 "Editor: Loaded scene from..." 消息
   - 查看是否有 "Editor: Warning - No render commands generated" 警告

2. **验证场景加载**：
   - 确保 `build/Package/Scenes/example_scene.json` 存在
   - 或手动加载场景文件

3. **检查渲染循环**：
   - 确认 `OnCompositionTargetRendering` 被调用
   - 确认 `BeginFrame`, `RenderViewport`, `EndFrame` 被调用

## 下一步改进

1. **修复 Swapchain 表面问题**：
   - 实现从 HWND 创建 Vulkan 表面
   - 为视口窗口创建独立的 swapchain

2. **添加场景加载 UI**：
   - 实现场景加载对话框
   - 支持拖放场景文件

3. **改进错误处理**：
   - 添加更详细的错误日志
   - 显示渲染状态信息

4. **优化渲染性能**：
   - 避免每帧重建 FrameGraph（如果配置未改变）
   - 实现帧率限制
