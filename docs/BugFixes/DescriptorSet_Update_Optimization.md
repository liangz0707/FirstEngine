# Descriptor Set 更新优化说明

## 日期
2024年（当前会话）

## 问题概述

在渲染过程中，`FlushParametersToGPU` 和 `UpdateBindings` 可能被多次调用，导致不必要的 descriptor set 更新，从而引发 Vulkan validation 错误。

## 重复调用的原因

### 1. 多个 Entity 共享同一个 Material

**场景**：
- 多个 `Entity` 使用同一个 `MaterialResource`
- 每个 `Entity` 都有自己的 `ModelComponent`
- 每个 `ModelComponent` 通过 `GetShadingMaterial()` 获取同一个 `ShadingMaterial` 实例

**调用流程**：
```
BuildRenderQueueFromEntities()
  └─> for each visibleEntity:
        └─> EntityToRenderItems(entity)
              └─> for each component:
                    └─> GetShadingMaterial()
                    └─> FlushParametersToGPU()  // 每个 entity 都会调用
                          └─> UpdateBindings()   // 每个 entity 都会调用
```

**示例**：
- Entity A 使用 Material M → 调用 `FlushParametersToGPU(M)`
- Entity B 使用 Material M → 调用 `FlushParametersToGPU(M)` （重复）
- Entity C 使用 Material M → 调用 `FlushParametersToGPU(M)` （重复）

### 2. 同一个 Entity 有多个 Component

**场景**：
- 一个 `Entity` 有多个 `Component`，都使用同一个 `MaterialResource`

**调用流程**：
```
EntityToRenderItems(entity)
  └─> for each component:
        └─> GetShadingMaterial()  // 可能返回同一个实例
        └─> FlushParametersToGPU()  // 每个 component 都会调用
```

### 3. 递归处理子 Entity

**场景**：
- 父 `Entity` 和子 `Entity` 都使用同一个 `MaterialResource`

**调用流程**：
```
EntityToRenderItems(parentEntity)
  └─> FlushParametersToGPU(material)  // 父 entity
  └─> for each child:
        └─> EntityToRenderItems(child)
              └─> FlushParametersToGPU(material)  // 子 entity（重复）
```

## 为什么需要多次调用 FlushParametersToGPU？

虽然 `FlushParametersToGPU` 被多次调用，但这是**必要的**，因为：

1. **Per-Object Uniform Buffer 数据不同**：
   - 每个 `Entity` 的 `modelMatrix`、`normalMatrix` 等数据不同
   - 需要为每个 `Entity` 更新 uniform buffer 内容

2. **Per-Frame Uniform Buffer 数据相同**：
   - `viewMatrix`、`projectionMatrix` 等数据对所有 `Entity` 相同
   - 但每个 `Entity` 仍然会更新（虽然数据相同）

3. **Texture Bindings 通常不变**：
   - Texture 指针通常不会改变
   - 只需要在第一次调用时更新 descriptor set binding

## 优化方案

### 1. Per-Frame 更新跟踪

**实现**：
- 在 `MaterialDescriptorManager` 中添加 `m_UpdatedThisFrame` 集合
- 跟踪本帧已更新的 descriptor sets
- 在 `BeginFrame()` 时清除跟踪

**效果**：
- 每个 descriptor set 在每帧中最多更新一次
- 避免在 command buffer 使用期间更新 descriptor set

**代码位置**：
- `include/FirstEngine/Renderer/MaterialDescriptorManager.h`
- `src/Renderer/MaterialDescriptorManager.cpp`

### 2. Texture Pointer 缓存

**实现**：
- 在 `MaterialDescriptorManager` 中添加 `m_LastTexturePointers` 映射
- 跟踪每个 binding 的上次 texture 指针
- 如果 texture 指针未改变，跳过 descriptor set 更新

**效果**：
- 只在 texture 指针改变时更新 descriptor set binding
- 减少不必要的更新

**代码位置**：
- `include/FirstEngine/Renderer/MaterialDescriptorManager.h`
- `src/Renderer/MaterialDescriptorManager.cpp::WriteDescriptorSets()`

### 3. 跳过 Uniform Buffer Binding 更新

**实现**：
- 在 `UpdateBindings()` 中，跳过 uniform buffer binding 的更新
- Uniform buffer 内容通过 `UpdateData()` 更新，不需要更新 binding

**效果**：
- 避免更新正在使用的 uniform buffer bindings
- 减少 validation 错误

**代码位置**：
- `src/Renderer/MaterialDescriptorManager.cpp::UpdateBindings()`
- `src/Renderer/MaterialDescriptorManager.cpp::WriteDescriptorSets()`

## 调用时机

### BeginFrame
- **位置**：`SceneRenderer::BuildRenderQueueFromEntities()` 开始时
- **作用**：清除所有 `ShadingMaterial` 的 per-frame 跟踪
- **时机**：每帧开始时，在任何 `FlushParametersToGPU` 调用之前

### FlushParametersToGPU
- **位置**：`SceneRenderer::EntityToRenderItems()` 中
- **作用**：
  1. 更新 uniform buffer 内容（每个 entity 都需要）
  2. 更新 descriptor set bindings（如果 texture 指针改变）
- **时机**：在创建 render item 之前，在 `SubmitRenderQueue` 之前

### UpdateBindings
- **位置**：`ShadingMaterial::FlushParametersToGPU()` 中
- **作用**：更新 descriptor set bindings（仅 texture bindings）
- **时机**：在 uniform buffer 内容更新之后

## 数据流程

```
每帧开始:
  BeginFrame() → 清除 m_UpdatedThisFrame

对每个 Entity:
  EntityToRenderItems()
    └─> FlushParametersToGPU()
          └─> DoUpdate() → 更新 uniform buffer 内容（每个 entity 都需要）
          └─> UpdateBindings()
                └─> WriteDescriptorSets()
                      └─> 检查 m_UpdatedThisFrame → 如果已更新，跳过
                      └─> 检查 texture pointer → 如果未改变，跳过
                      └─> 更新 descriptor set → 标记为已更新
```

## 性能考虑

### 当前实现
- ✅ 每个 entity 更新自己的 per-object uniform buffer（必要）
- ✅ Descriptor set binding 最多更新一次 per frame（优化）
- ✅ Texture binding 只在指针改变时更新（优化）

### 潜在优化
1. **Per-Frame Uniform Buffer 共享**：
   - 所有 entity 共享同一个 per-frame uniform buffer
   - 只在每帧开始时更新一次
   - 需要重构 uniform buffer 管理

2. **Material 实例化**：
   - 为每个 entity 创建独立的 material 实例
   - 避免共享 material 导致的重复更新
   - 增加内存开销

3. **延迟更新**：
   - 收集所有需要更新的 descriptor sets
   - 在 command buffer 录制之前批量更新
   - 需要更复杂的同步机制

## 总结

重复调用 `FlushParametersToGPU` 是**正常且必要的**，因为：
1. 每个 entity 的 per-object 数据不同，需要单独更新
2. 当前的优化机制（per-frame 跟踪、texture pointer 缓存）已经防止了不必要的 descriptor set 更新
3. Validation 错误已经通过 per-frame 跟踪机制解决

如果仍然看到 validation 错误，可能是：
- `BeginFrame()` 没有被正确调用
- Descriptor set 在 command buffer 录制之后被更新
- 需要检查调用顺序和时机
