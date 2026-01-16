# FirstEngine 渲染流程文档

## 概述

FirstEngine 的渲染流程已经重构为三个阶段，实现了场景遍历和 CommandBuffer 记录指令的完全分离，为多线程优化提供了基础。

## 渲染流程三个阶段

### 阶段1: OnUpdate() - 游戏逻辑更新
- 更新游戏状态
- 物理模拟
- 输入处理
- 动画更新

### 阶段2: OnRender() - 场景遍历和渲染数据收集
- **只进行场景遍历**
- **构建渲染队列（RenderQueue）**
- **不涉及任何 CommandBuffer 操作**
- 可以并行执行或多线程优化

**关键特性：**
- 不需要 CommandBuffer
- 不需要 Swapchain
- 不需要同步对象（Fence、Semaphore）
- 纯 CPU 操作，可以并行化

### 阶段3: OnSubmit() - CommandBuffer 记录和提交
- 等待上一帧完成（Fence）
- 获取 Swapchain 图像
- 创建 CommandBuffer
- 开始录制
- **记录渲染队列到 CommandBuffer**
- 执行 FrameGraph
- 结束录制
- 提交到 GPU
- Present

**关键特性：**
- 所有 CommandBuffer 操作集中在此
- 需要 GPU 资源（Swapchain、CommandBuffer）
- 需要同步对象

## 执行流程

```
MainLoop()
  │
  ├─ OnUpdate(deltaTime)
  │   └─ 游戏逻辑更新
  │
  ├─ OnRender()                    // 阶段2: 场景遍历
  │   ├─ Clear RenderQueue
  │   ├─ 计算 View/Projection 矩阵
  │   └─ SceneRenderer::Render()   // 只构建渲染队列，不记录指令
  │       └─ BuildRenderQueue()    // 场景遍历，收集渲染数据
  │
  └─ OnSubmit()                    // 阶段3: CommandBuffer 记录和提交
      ├─ WaitForFence()            // 等待上一帧
      ├─ AcquireNextImage()        // 获取 Swapchain 图像
      ├─ CreateCommandBuffer()     // 创建 CommandBuffer
      ├─ Begin()                   // 开始录制
      ├─ TransitionImageLayout()   // 图像布局转换（开始）
      ├─ SubmitRenderQueue()       // 记录渲染队列到 CommandBuffer
      │   └─ 遍历 RenderQueue，记录 Draw 指令
      ├─ FrameGraph::Execute()     // 执行 FrameGraph
      ├─ TransitionImageLayout()   // 图像布局转换（结束）
      ├─ End()                     // 结束录制
      ├─ SubmitCommandBuffer()     // 提交到 GPU
      └─ Present()                 // 呈现
```

## 多线程优化潜力

### 当前架构的优势

1. **场景遍历可以并行化**
   - OnRender() 中只进行场景遍历，没有 GPU 资源依赖
   - 可以分割场景到多个线程并行遍历
   - 每个线程构建自己的 RenderQueue，最后合并

2. **CommandBuffer 记录可以优化**
   - 场景遍历完成后，再统一记录指令
   - 可以在不同线程中记录不同的 RenderQueue 部分
   - 最后合并多个 CommandBuffer 或使用 Secondary CommandBuffer

3. **清晰的职责分离**
   - 场景遍历：纯 CPU 操作，可并行
   - 指令记录：需要 CommandBuffer，但可以优化
   - 提交：GPU 操作，需要同步

### 未来多线程优化方向

1. **并行场景遍历**
   ```
   OnRender()
     ├─ Thread 1: 遍历场景区域 A → RenderQueue A
     ├─ Thread 2: 遍历场景区域 B → RenderQueue B
     ├─ Thread 3: 遍历场景区域 C → RenderQueue C
     └─ 合并 RenderQueue A + B + C → m_RenderQueue
   ```

2. **并行 CommandBuffer 记录**
   ```
   OnSubmit()
     ├─ 创建 Primary CommandBuffer
     ├─ Thread 1: 记录 RenderQueue A → Secondary CommandBuffer 1
     ├─ Thread 2: 记录 RenderQueue B → Secondary CommandBuffer 2
     ├─ Thread 3: 记录 RenderQueue C → Secondary CommandBuffer 3
     └─ 执行所有 Secondary CommandBuffers
   ```

3. **异步场景遍历**
   - 在上一帧的 OnSubmit() 执行时，下一帧的 OnRender() 可以并行执行
   - 需要确保数据同步和一致性

## 关键数据结构

### RenderQueue
- 存储场景遍历收集的渲染数据
- 包含 RenderItem 列表
- 自动批处理（按 Pipeline、Material 分组）
- 线程安全（如果多线程访问，需要加锁）

### RenderItem
- 单个 DrawCall 的数据
- 包含几何信息（VertexBuffer、IndexBuffer）
- 包含 Pipeline 状态
- 包含变换矩阵
- 包含材质信息

## 注意事项

1. **线程安全**
   - 当前 `m_RenderQueue` 是成员变量，如果多线程访问需要加锁
   - 建议每个线程使用独立的 RenderQueue，最后合并

2. **数据一致性**
   - OnRender() 和 OnSubmit() 之间，场景数据不应该被修改
   - 如果需要修改，应该在 OnUpdate() 中进行

3. **资源生命周期**
   - RenderQueue 中的资源引用（Buffer、Image）必须保证在提交时仍然有效
   - 避免在 OnRender() 和 OnSubmit() 之间释放资源

## 性能考虑

1. **场景遍历性能**
   - 使用八叉树等空间索引加速遍历
   - 视锥剔除减少不必要的处理
   - 遮挡剔除（可选）

2. **渲染队列构建**
   - 自动批处理减少 DrawCall
   - 排序优化 GPU 状态切换

3. **CommandBuffer 记录**
   - 批量记录指令
   - 减少状态切换
   - 使用 Secondary CommandBuffer 并行记录
