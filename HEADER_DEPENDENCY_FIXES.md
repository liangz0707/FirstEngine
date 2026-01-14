# 头文件依赖关系修复记录

## 修复的问题

### 1. IDevice.h 缺少 IImageView 的完整定义
**问题**: `IDevice.h` 使用了 `IImageView*`，但只有前向声明，导致编译错误
**修复**: 添加 `#include "FirstEngine/RHI/IImage.h"` 以获取 `IImageView` 的完整定义

**文件**: `include/FirstEngine/RHI/IDevice.h`
```cpp
// 修复前
#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

// 修复后
#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IImage.h"  // 需要 IImageView 的完整定义
```

### 2. VulkanRHIWrappers.h 缺少 RHI 接口的完整定义
**问题**: `VulkanRHIWrappers.h` 中的类继承自 RHI 接口，但只包含了 `IDevice.h` 和 `Types.h`，缺少其他接口的完整定义
**修复**: 添加所有需要的 RHI 接口头文件

**文件**: `include/FirstEngine/Device/VulkanRHIWrappers.h`
```cpp
// 修复后添加的包含
#include "FirstEngine/RHI/ICommandBuffer.h"  // 需要 ICommandBuffer 的完整定义
#include "FirstEngine/RHI/IRenderPass.h"      // 需要 IRenderPass 的完整定义
#include "FirstEngine/RHI/IFramebuffer.h"     // 需要 IFramebuffer 的完整定义
#include "FirstEngine/RHI/IPipeline.h"         // 需要 IPipeline 的完整定义
#include "FirstEngine/RHI/IBuffer.h"          // 需要 IBuffer 的完整定义
#include "FirstEngine/RHI/IImage.h"            // 需要 IImage 和 IImageView 的完整定义
#include "FirstEngine/RHI/ISwapchain.h"        // 需要 ISwapchain 的完整定义
#include "FirstEngine/RHI/IShaderModule.h"    // 需要 IShaderModule 的完整定义
```

### 3. Pipeline.h 不必要的 RenderPass.h 包含
**问题**: `Pipeline.h` 包含了 `RenderPass.h`，但 `GraphicsPipelineDescription` 只使用 `RenderPass*` 指针，不需要完整定义
**修复**: 移除 `RenderPass.h` 的包含，使用前向声明

**文件**: `include/FirstEngine/Device/Pipeline.h`
```cpp
// 修复前
#include "FirstEngine/Device/RenderPass.h"

// 修复后
// 移除包含，使用前向声明
class RenderPass;  // 前向声明
```

### 4. Framebuffer.h 不必要的 RenderPass.h 包含
**问题**: `Framebuffer.h` 包含了 `RenderPass.h`，但只使用 `RenderPass*` 指针
**修复**: 移除包含，使用前向声明

**文件**: `include/FirstEngine/Device/Framebuffer.h`
```cpp
// 修复前
#include "FirstEngine/Device/RenderPass.h"

// 修复后
// 前向声明
class RenderPass;
```

### 5. PipelineConfig.h 循环依赖
**问题**: `PipelineConfig.h` 包含了 `FrameGraph.h`，但 `PipelineConfig` 结构体不依赖 `FrameGraph`
**修复**: 移除不必要的包含

**文件**: `include/FirstEngine/Renderer/PipelineConfig.h`
```cpp
// 修复前
#include "FirstEngine/Renderer/FrameGraph.h"

// 修复后
// 移除包含，PipelineConfig 不依赖 FrameGraph
```

## 依赖关系原则

### 正确的包含顺序
1. **当前文件的对应头文件**（如果有）
2. **系统头文件** (`<...>`)
3. **项目头文件** (`"..."`)
   - 先包含基础类型和接口
   - 再包含具体实现

### 何时使用前向声明
- ✅ 只使用指针或引用时，使用前向声明
- ✅ 在函数参数中使用指针/引用时，使用前向声明
- ❌ 需要访问类的成员或方法时，必须包含完整定义
- ❌ 继承自某个类时，必须包含完整定义
- ❌ 使用 `std::unique_ptr<T>` 或 `std::shared_ptr<T>` 时，通常需要完整定义（除非在实现文件中）

### 避免循环依赖
- 如果 A.h 包含 B.h，B.h 不应包含 A.h
- 使用前向声明打破循环依赖
- 将共同依赖提取到单独的头文件中

## 检查清单

在添加新的头文件包含时，检查：
- [ ] 是否真的需要完整类型定义？
- [ ] 是否可以使用前向声明？
- [ ] 是否会造成循环依赖？
- [ ] 包含顺序是否正确？

## 已修复的文件列表

1. ✅ `include/FirstEngine/RHI/IDevice.h` - 添加 IImage.h
2. ✅ `include/FirstEngine/Device/VulkanRHIWrappers.h` - 添加所有 RHI 接口头文件
3. ✅ `include/FirstEngine/Device/Pipeline.h` - 移除 RenderPass.h，使用前向声明
4. ✅ `include/FirstEngine/Device/Framebuffer.h` - 移除 RenderPass.h，使用前向声明
5. ✅ `include/FirstEngine/Renderer/PipelineConfig.h` - 移除 FrameGraph.h
6. ✅ `include/FirstEngine/Renderer/PipelineBuilder.h` - 添加 Types.h（使用了 RHI::Format）

## 注意事项

- `Types.h` 中的 `GraphicsPipelineDescription` 和 `ComputePipelineDescription` 使用 `IRenderPass*` 和 `IShaderModule*` 指针，这些类型在 `Types.h` 中有前向声明，这是正确的做法
- 当需要完整类型定义时（如继承、调用方法），必须包含对应的头文件
- 保持头文件包含的最小化，减少编译时间和依赖复杂度
