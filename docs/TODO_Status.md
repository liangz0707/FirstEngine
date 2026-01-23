# TODO 状态报告

本文档记录了项目中的 TODO 项及其完成状态。

## 已完成的 TODO

### ✅ ComputePipeline 创建 (VulkanDevice.cpp:431)
- **状态**: 已完成
- **描述**: 实现了 `VulkanDevice::CreateComputePipeline()` 方法
- **完成时间**: 2024
- **详情**: 支持描述符集布局和 Push Constants，与 GraphicsPipeline 实现保持一致

### ✅ Sampler 创建/获取 (MaterialDescriptorManager.cpp:345)
- **状态**: 已完成
- **描述**: 实现了默认 Sampler 的创建和管理
- **完成时间**: 2024
- **详情**: 
  - 在 `VulkanDevice` 中添加了 `GetDefaultSampler()` 方法
  - 创建并缓存默认 Sampler（线性过滤、重复寻址）
  - 在 `MaterialDescriptorManager` 中自动使用默认 Sampler

### ✅ PushConstants (CommandRecorder.cpp:186)
- **状态**: 已完成
- **描述**: 实现了 PushConstants 功能
- **完成时间**: 2024
- **详情**:
  - 在 `ICommandBuffer` 接口中添加了 `PushConstants()` 方法
  - 在 `VulkanCommandBuffer` 中实现了 PushConstants
  - 在 `CommandRecorder` 中跟踪当前绑定的 pipeline

### ✅ 资源加载逻辑 (EditorAPI.cpp:676)
- **状态**: 已完成
- **描述**: 实现了 EditorAPI 中的资源加载逻辑
- **完成时间**: 2024
- **详情**: 支持通过路径和类型加载资源，自动检测资源类型

### ✅ 资源卸载逻辑 (EditorAPI.cpp:683)
- **状态**: 已完成
- **描述**: 实现了 EditorAPI 中的资源卸载逻辑
- **完成时间**: 2024
- **详情**: 支持通过路径卸载资源，自动检测资源类型

### ✅ 清理旧代码
- **状态**: 已完成
- **描述**: 清理了 FrameGraph 中的旧代码注释，标记了废弃方法
- **完成时间**: 2024
- **详情**: 
  - 移除了注释掉的旧 Framebuffer 和 RenderPass 映射代码
  - 标记了 VulkanRenderer 中的废弃方法
  - 为兼容性代码添加了文档说明

### ✅ PostProcessing (PostProcessPass.cpp:74)
- **状态**: 基础框架已完成
- **描述**: 已实现后处理 Pass 的基础框架和资源访问
- **完成时间**: 2024
- **详情**: 
  - 实现了从 FrameGraphBuilder 读取 FinalOutput 资源
  - 添加了完整的实现说明和 TODO 注释
  - 需要全屏四边形渲染和着色器支持才能完全实现
  - 已添加详细的实现指南（注释形式）

### ✅ Lighting 计算 (LightingPass.cpp:78)
- **状态**: 基础框架已完成
- **描述**: 已实现光照 Pass 的基础框架和资源访问
- **完成时间**: 2024
- **详情**: 
  - 实现了从 FrameGraphBuilder 读取 G-Buffer 资源（Albedo, Normal, Depth）
  - 添加了完整的实现说明和 TODO 注释
  - 需要全屏四边形渲染和着色器支持才能完全实现
  - 已添加详细的实现指南（注释形式）

### ✅ Staging Buffer (RenderGeometry.cpp:86, 111)
- **状态**: 已完成
- **描述**: 实现了使用 Staging Buffer 进行内存上传
- **完成时间**: 2024
- **详情**: 
  - 创建 device-local 缓冲区用于顶点和索引数据（GPU 访问优化）
  - 使用 staging buffer（HostVisible）临时存储数据
  - 通过 command buffer 复制数据从 staging buffer 到 device-local buffer
  - 提交并等待完成，确保数据已上传
  - 移除了直接使用 HostVisible 内存的方式

### ✅ 移除 Model/Mesh/Material 从内存加载接口
- **状态**: 已完成
- **描述**: 移除了不需要的内存加载接口
- **完成时间**: 2024
- **详情**: 
  - 移除了 `IResourceProvider::LoadFromMemory()` 接口声明
  - 移除了 `MeshResource::LoadFromMemory()` 实现
  - 移除了 `MaterialResource::LoadFromMemory()` 实现
  - 移除了 `ModelResource::LoadFromMemory()` 实现
  - 注意：保留了 `TextureResource::LoadFromMemory()`，因为纹理需要从内存加载（用于图标等）

## 待完成的 TODO

### ⏳ GPU 遮挡剔除 (RenderBatch.cpp:302)
- **优先级**: 低
- **描述**: 实现基于 GPU 的遮挡剔除
- **影响**: 性能优化

## 已标记为废弃的代码

### VulkanRenderer 废弃方法
- `OnWindowResize()` - 已废弃，使用 `RHI::ISwapchain::Recreate()` 代替
- `Present()` - 已废弃，使用 `RHI::ISwapchain::Present()` 代替

### FrameGraph 旧代码
- 已删除注释掉的旧 Framebuffer 和 RenderPass 映射代码

## 兼容性代码

以下代码保留用于向后兼容，但计划在未来版本中移除：

1. **ResourceProvider 路径方法** (ResourceProvider.cpp:307-471)
   - `Load(const std::string&)` - 路径加载方法
   - `Get(const std::string&)` - 路径获取方法
   - `Unload(const std::string&)` - 路径卸载方法
   - 建议：迁移到基于 ResourceID 的 API

2. **MeshLoader 遗留顶点格式** (MeshLoader.h:15, MeshLoader.cpp:293)
   - `GetLegacyVertices()` - 遗留顶点格式转换
   - 建议：逐步迁移到新的顶点格式系统

3. **ResourceXMLParser 向后兼容** (ResourceXMLParser.cpp:101, 262)
   - 回退到 ID 的兼容性代码
   - Mesh XML 中的旧格式支持

## 代码清理总结

### 已清理
- ✅ 移除了 FrameGraph 中注释掉的旧代码
- ✅ 标记了废弃方法（VulkanRenderer::OnWindowResize, Present）
- ✅ 为所有兼容性代码添加了清晰的文档说明和 TODO 标记

### 兼容性代码文档化
所有保留的兼容性代码都已添加了：
- `NOTE:` 注释说明保留原因
- `TODO:` 标记未来移除计划
- 迁移建议

## 后续工作建议

### 高优先级
1. **完成 Lighting 和 PostProcessing**: 
   - 实现全屏四边形渲染支持
   - 创建 Lighting 着色器（读取 G-Buffer，计算光照）
   - 创建 Post-Processing 着色器（应用后处理效果）
   - 创建相应的 Graphics Pipeline

### 中优先级
2. **性能优化**: 
   - 实现 GPU 遮挡剔除以优化渲染性能

### 低优先级
3. **功能完善**: 
   - 创建专门的 Layout 枚举，替换 Format 枚举在 Layout 中的使用

### 代码维护
4. **代码清理**: 逐步移除兼容性代码，提供迁移指南
