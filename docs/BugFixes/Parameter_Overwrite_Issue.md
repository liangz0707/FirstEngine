# 参数覆盖问题分析

## 日期
2024年（当前会话）

## 问题概述

当多个 Entity 共享同一个 `ShadingMaterial` 实例时，后面的 Entity 会覆盖前面 Entity 的 per-object uniform buffer 数据（如 `modelMatrix`、`normalMatrix`），导致渲染错误。

## 问题分析

### 当前实现流程

```
对每个 Entity:
  EntityToRenderItems(entity)
    └─> ApplyParameters(collector) → 设置 m_RenderParameters["modelMatrix"] = matrixA
    └─> UpdateRenderParameters() → 将 matrixA 写入 uniform buffer 的 CPU 数据 (ub->data)
    └─> FlushParametersToGPU() → 将 uniform buffer 的 CPU 数据更新到 GPU
```

### 问题场景

**场景**：Entity A 和 Entity B 共享同一个 `ShadingMaterial` 实例

**执行流程**：
1. Entity A:
   - `ApplyParameters(collectorA)` → `m_RenderParameters["modelMatrix"] = matrixA`
   - `UpdateRenderParameters()` → `ub->data` 包含 `matrixA`
   - `FlushParametersToGPU()` → GPU uniform buffer 包含 `matrixA`

2. Entity B:
   - `ApplyParameters(collectorB)` → **覆盖** `m_RenderParameters["modelMatrix"] = matrixB`
   - `UpdateRenderParameters()` → **覆盖** `ub->data` 包含 `matrixB`（覆盖 `matrixA`）
   - `FlushParametersToGPU()` → GPU uniform buffer 包含 `matrixB`（覆盖 `matrixA`）

3. 绘制时：
   - Entity A 和 Entity B 使用同一个 descriptor set，指向同一个 uniform buffer
   - Entity A 绘制时，uniform buffer 包含的是 `matrixB`（错误！）
   - Entity B 绘制时，uniform buffer 包含的是 `matrixB`（正确）

### 根本原因

1. **共享的 Uniform Buffer**：
   - 所有 Entity 共享同一个 `ShadingMaterial` 实例
   - 所有 Entity 共享同一个 uniform buffer 实例（`ub->buffer`）
   - 所有 Entity 共享同一个 uniform buffer 的 CPU 数据（`ub->data`）

2. **参数覆盖**：
   - `ApplyParameters()` 直接覆盖 `m_RenderParameters[key]`
   - `UpdateRenderParameters()` 直接覆盖 uniform buffer 的 CPU 数据
   - 没有为每个 Entity 维护独立的 per-object 数据

3. **绘制时机**：
   - Uniform buffer 在 `EntityToRenderItems` 中更新
   - 绘制在 `SubmitRenderQueue` 中进行
   - 在绘制时，uniform buffer 已经包含最后一个 Entity 的数据

## 解决方案

### 方案 1：延迟更新 Uniform Buffer（推荐）

**原理**：
- 不在 `EntityToRenderItems` 中更新 uniform buffer
- 在 `SubmitRenderQueue` 中，绘制每个 item 之前立即更新 uniform buffer
- 这样每个 Entity 在绘制时都有正确的数据

**优点**：
- 不需要修改 uniform buffer 管理
- 不需要为每个 Entity 创建独立的 uniform buffer
- 实现简单

**缺点**：
- 需要确保 uniform buffer 在绘制之前更新
- 可能增加绘制时的 CPU 开销

**实现**：
```cpp
// 在 SubmitRenderQueue 中，绘制每个 item 之前
for (const auto& item : items) {
    auto* shadingMaterial = static_cast<ShadingMaterial*>(item.materialData.shadingMaterial);
    if (shadingMaterial) {
        // 立即更新 per-object uniform buffer 数据
        // 使用 RenderItem 中存储的 worldMatrix
        RenderParameterCollector collector;
        collector.CollectFromComponent(item.entity->GetComponent<ModelComponent>(), item.entity);
        shadingMaterial->ApplyParameters(collector);
        shadingMaterial->UpdateRenderParameters();
        shadingMaterial->FlushParametersToGPU(m_Device);
    }
    
    // 然后绘制
    cmd->DrawIndexed(...);
}
```

### 方案 2：使用 Push Constants

**原理**：
- Per-object 数据（如 `modelMatrix`）通过 push constants 传递
- Uniform buffer 只存储 per-material 和 per-frame 数据
- 每个 Entity 在绘制时设置自己的 push constants

**优点**：
- 避免 uniform buffer 更新冲突
- 性能更好（push constants 在 GPU 寄存器中）
- 每个 Entity 独立设置数据

**缺点**：
- 需要修改着色器以使用 push constants
- Push constants 大小有限（通常 128 字节）

**实现**：
```cpp
// 在 SubmitRenderQueue 中，绘制每个 item 之前
for (const auto& item : items) {
    // 设置 push constants
    RenderCommand pushCmd;
    pushCmd.type = RenderCommandType::PushConstants;
    pushCmd.params.pushConstants.pipeline = itemPipeline;
    pushCmd.params.pushConstants.stageFlags = RHI::ShaderStage::Vertex;
    pushCmd.params.pushConstants.offset = 0;
    pushCmd.params.pushConstants.size = sizeof(glm::mat4) * 2; // modelMatrix + normalMatrix
    pushCmd.params.pushConstants.data = &item.worldMatrix; // 使用 RenderItem 中的 worldMatrix
    commandList.AddCommand(std::move(pushCmd));
    
    // 然后绘制
    cmd->DrawIndexed(...);
}
```

### 方案 3：每个 Entity 独立的 Uniform Buffer 实例

**原理**：
- 为每个 Entity 创建独立的 uniform buffer 实例
- 每个 Entity 有自己的 descriptor set（使用 dynamic uniform buffers）
- 或者使用 uniform buffer arrays

**优点**：
- 完全避免参数覆盖
- 支持每个 Entity 独立的数据

**缺点**：
- 需要大量内存（每个 Entity 一个 uniform buffer）
- 需要修改 descriptor set 管理
- 实现复杂

## 当前实现的问题

### 问题 1：参数覆盖

**代码位置**：`src/Renderer/ShadingMaterial.cpp::ApplyParameters()`
```cpp
void ShadingMaterial::ApplyParameters(const RenderParameterCollector& collector) {
    for (const auto& [key, value] : params) {
        SetRenderParameter(key, materialValue); // 直接覆盖 m_RenderParameters[key]
    }
}
```

**问题**：如果多个 Entity 共享同一个 material，后面的 Entity 会覆盖前面 Entity 的参数。

### 问题 2：Uniform Buffer 数据覆盖

**代码位置**：`src/Renderer/ShadingMaterial.cpp::UpdateRenderParameters()`
```cpp
bool ShadingMaterial::UpdateRenderParameters() {
    for (const auto& [key, value] : m_RenderParameters) {
        ApplyRenderParameter(key, value, true); // 直接覆盖 uniform buffer 的 CPU 数据
    }
}
```

**问题**：如果多个 Entity 共享同一个 material，后面的 Entity 会覆盖前面 Entity 的 uniform buffer 数据。

### 问题 3：GPU 数据覆盖

**代码位置**：`src/Renderer/ShadingMaterial.cpp::FlushParametersToGPU()`
```cpp
bool ShadingMaterial::FlushParametersToGPU(RHI::IDevice* device) {
    // ...
    DoUpdate(device); // 将 uniform buffer 的 CPU 数据更新到 GPU
    // ...
}
```

**问题**：如果多个 Entity 共享同一个 material，后面的 Entity 会覆盖前面 Entity 的 GPU uniform buffer 数据。

## 推荐的修复方案

### 方案 1：延迟更新（最简单）

**修改**：在 `SubmitRenderQueue` 中，绘制每个 item 之前立即更新 uniform buffer

**优点**：
- 实现简单
- 不需要修改 uniform buffer 管理
- 不需要修改着色器

**缺点**：
- 可能增加绘制时的 CPU 开销
- 需要确保更新时机正确

### 方案 2：使用 Push Constants（性能最好）

**修改**：
1. 修改着色器以使用 push constants 传递 per-object 数据
2. 在 `SubmitRenderQueue` 中，绘制每个 item 之前设置 push constants

**优点**：
- 性能最好
- 完全避免参数覆盖
- 每个 Entity 独立设置数据

**缺点**：
- 需要修改着色器
- Push constants 大小有限

## 当前 Per-Frame 跟踪的影响

当前的 per-frame 跟踪机制（`m_UpdatedThisFrame`）**不会**解决参数覆盖问题，因为：

1. Per-frame 跟踪只防止 descriptor set binding 的重复更新
2. 它不防止 uniform buffer **内容**的覆盖
3. Uniform buffer 内容通过 `UpdateData()` 更新，不受 per-frame 跟踪影响

## 总结

**是的，当前的实现会导致参数覆盖问题**。如果多个 Entity 共享同一个 `ShadingMaterial`，后面的 Entity 会覆盖前面 Entity 的 per-object uniform buffer 数据。

**建议**：
1. 短期：使用方案 1（延迟更新），在绘制时立即更新 uniform buffer
2. 长期：使用方案 2（push constants），将 per-object 数据通过 push constants 传递
