# FirstEngine RHI 架构文档

## 概述

FirstEngine 采用分层架构设计，将渲染管线与底层图形 API 解耦，支持多种图形 API（Vulkan、DirectX 11、Metal、OpenGL）。

## 架构层次

```
┌─────────────────────────────────────┐
│   FirstEngine_Renderer              │
│   (FrameGraph 渲染管线定义)          │
│   - 与图形API无关                    │
│   - 使用FrameGraph策略               │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   FirstEngine_RHI                   │
│   (渲染硬件接口抽象层)                │
│   - IDevice 接口                     │
│   - ICommandBuffer, IRenderPass等    │
│   - 资源抽象 (IBuffer, IImage等)     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   FirstEngine_Device                │
│   (Vulkan实现)                       │
│   - VulkanDevice (实现IDevice)       │
│   - VulkanCommandBuffer等            │
│   - 可扩展支持DX11/Metal/OpenGL      │
└─────────────────────────────────────┘
```

## 模块说明

### FirstEngine_RHI

**位置**: `include/FirstEngine/RHI/`, `src/RHI/`

**职责**: 定义渲染硬件接口抽象层，提供与图形 API 无关的接口。

**核心接口**:
- `IDevice` - 设备接口，所有图形 API 的抽象
- `ICommandBuffer` - 命令缓冲区接口
- `IRenderPass` - 渲染通道接口
- `IFramebuffer` - 帧缓冲区接口
- `IPipeline` - 管道接口
- `IBuffer` - 缓冲区接口
- `IImage` - 图像接口
- `ISwapchain` - 交换链接口
- `IShaderModule` - 着色器模块接口

**类型定义** (`Types.h`):
- `Format`, `BufferUsageFlags`, `MemoryPropertyFlags` 等枚举
- `RenderPassDescription`, `GraphicsPipelineDescription` 等结构体
- `DeviceInfo` 等设备信息结构

### FirstEngine_Renderer

**位置**: `include/FirstEngine/Renderer/`, `src/Renderer/`

**职责**: 实现与图形 API 无关的渲染管线创建代码，使用 FrameGraph 策略。

**核心类**:
- `FrameGraph` - FrameGraph 主类，管理渲染管线
- `FrameGraphNode` - FrameGraph 节点（渲染通道）
- `FrameGraphResource` - FrameGraph 资源（纹理、缓冲区等）
- `FrameGraphBuilder` - FrameGraph 构建器，用于在节点执行时访问资源

**FrameGraph 工作流程**:
1. **构建阶段**: 添加节点和资源，定义依赖关系
2. **编译阶段**: 分析依赖，分配资源，拓扑排序
3. **执行阶段**: 按顺序执行节点，生成渲染命令

### FirstEngine_Device

**位置**: `include/FirstEngine/Device/`, `src/Device/`

**职责**: 实现 Vulkan 版本的 RHI 接口。

**核心类**:
- `VulkanDevice` - 实现 `IDevice` 接口的 Vulkan 版本
- `VulkanRenderer` - Vulkan 渲染器（底层实现）
- `DeviceContext` - Vulkan 设备上下文
- `CommandBuffer`, `RenderPass`, `Framebuffer` 等 - Vulkan 对象封装

**扩展性**: 
- 可以添加 `D3D11Device` 实现 DirectX 11
- 可以添加 `MetalDevice` 实现 Metal
- 可以添加 `OpenGLDevice` 实现 OpenGL

## 使用示例

### 1. 创建设备

```cpp
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Core/Window.h"

// 创建窗口
auto window = std::make_unique<FirstEngine::Core::Window>(1280, 720, "FirstEngine");

// 创建 Vulkan 设备
auto device = std::make_unique<FirstEngine::Device::VulkanDevice>();
device->Initialize(window->GetHandle());
```

### 2. 创建 FrameGraph

```cpp
#include "FirstEngine/Renderer/FrameGraph.h"

// 创建 FrameGraph
FirstEngine::Renderer::FrameGraph graph(device.get());

// 添加资源
ResourceDescription colorDesc;
colorDesc.type = ResourceType::Attachment;
colorDesc.width = 1280;
colorDesc.height = 720;
colorDesc.format = RHI::Format::B8G8R8A8_UNORM;
graph.AddResource("ColorAttachment", colorDesc);

// 添加节点
auto* node = graph.AddNode("RenderPass");
node->AddWriteResource("ColorAttachment");
node->SetExecuteCallback([](RHI::ICommandBuffer* cmd, FrameGraphBuilder& builder) {
    // 渲染逻辑
    auto* colorAttachment = builder.WriteTexture("ColorAttachment");
    // ...
});

// 编译和执行
graph.Compile();
graph.Execute(commandBuffer);
```

## 扩展指南

### 添加新的图形 API 支持

1. 在 `FirstEngine_Device` 中创建新的设备类（如 `D3D11Device`）
2. 实现 `IDevice` 接口的所有方法
3. 创建对应的资源包装类（如 `D3D11CommandBuffer`, `D3D11Buffer` 等）
4. 在 `FrameGraph` 的资源分配中支持新 API

### 自定义渲染管线

1. 在 `FirstEngine_Renderer` 中创建新的渲染管线类
2. 使用 `FrameGraph` 构建渲染流程
3. 定义节点和资源依赖关系
4. 编译和执行 FrameGraph

## 注意事项

1. **资源生命周期**: FrameGraph 会自动管理资源的生命周期，根据使用情况分配和释放
2. **线程安全**: 当前实现不是线程安全的，需要在多线程环境中添加同步机制
3. **错误处理**: 所有接口方法都应该返回适当的错误码或抛出异常
4. **性能**: FrameGraph 编译阶段会进行优化，但执行阶段需要确保命令缓冲区的高效使用

## 未来改进

- [ ] 实现完整的 Vulkan 接口包装
- [ ] 添加 DirectX 11 支持
- [ ] 添加 Metal 支持
- [ ] 添加 OpenGL 支持
- [ ] FrameGraph 优化（资源复用、异步执行等）
- [ ] 多线程渲染支持
- [ ] 渲染管线序列化/反序列化
