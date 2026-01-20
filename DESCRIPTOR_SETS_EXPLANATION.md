# 描述符集（Descriptor Sets）详解

## 1. 什么是描述符集（Descriptor Set）？

### 概念
描述符集是 Vulkan 渲染 API 中的一个核心概念，用于将 **Uniform Buffers（统一缓冲区）** 和 **Textures/Samplers（纹理/采样器）** 组织在一起，传递给 Shader（着色器）使用。

### 类比理解
可以把描述符集想象成一个"资源包"：
- **Descriptor Set Layout（描述符集布局）**：定义了资源包的结构（哪些位置放什么类型的资源）
- **Descriptor Set（描述符集）**：实际的资源包实例（包含了具体的 uniform buffer 和 texture）
- **Binding（绑定）**：资源在包中的位置编号

### Shader 中的对应关系

在 Shader 代码中，你会看到这样的声明：

```hlsl
// Set 0, Binding 0: Uniform Buffer
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Set 0, Binding 1: Texture
layout(set = 0, binding = 1) uniform texture2D albedoMap;
layout(set = 0, binding = 2) uniform sampler texSampler;
```

这里的 `set = 0` 对应一个描述符集，`binding = 0/1/2` 对应资源在这个描述符集中的位置。

## 2. CreateDescriptorSets 中的数据含义

### 当前实现（占位符）

```cpp
bool ShadingMaterial::CreateDescriptorSets(RHI::IDevice* device) {
    // 按 set 分组收集资源绑定信息
    std::map<uint32_t, std::vector<std::pair<uint32_t, std::string>>> setBindings;
    
    // 收集 Uniform Buffer 绑定信息
    for (const auto& ub : m_UniformBuffers) {
        setBindings[ub.set].push_back({ub.binding, "uniform_buffer"});
        // 例如：setBindings[0].push_back({0, "uniform_buffer"})
        // 表示：Set 0 的 Binding 0 位置是一个 uniform buffer
    }
    
    // 收集 Texture 绑定信息
    for (const auto& tb : m_TextureBindings) {
        setBindings[tb.set].push_back({tb.binding, "texture"});
        // 例如：setBindings[0].push_back({1, "texture"})
        // 表示：Set 0 的 Binding 1 位置是一个 texture
    }
    
    // 注意：当前实现只是收集信息，实际的描述符集创建是占位符
    // 真正的创建需要在设备实现层（如 VulkanDevice）完成
    return true;
}
```

### 数据结构说明

**`setBindings` 的结构：**
```cpp
std::map<uint32_t, std::vector<std::pair<uint32_t, std::string>>>
//        ↑set索引    ↑该set中的所有绑定    ↑binding索引   ↑资源类型
```

**示例数据：**
```cpp
setBindings[0] = [
    {0, "uniform_buffer"},  // Set 0, Binding 0: Uniform Buffer
    {1, "texture"},         // Set 0, Binding 1: Texture
    {2, "texture"}          // Set 0, Binding 2: Texture
]

setBindings[1] = [
    {0, "uniform_buffer"}   // Set 1, Binding 0: Uniform Buffer (可能是 per-frame 数据)
]
```

### 数据的来源

这些数据来自 **Shader Reflection（着色器反射）**：

1. **Uniform Buffers**：从 `ShaderReflection.uniform_buffers` 解析
   - 每个 uniform buffer 有 `set` 和 `binding` 信息
   - 存储在 `m_UniformBuffers` 中

2. **Textures/Samplers**：从 `ShaderReflection.samplers` 和 `ShaderReflection.images` 解析
   - 每个 texture/sampler 有 `set` 和 `binding` 信息
   - 存储在 `m_TextureBindings` 中

## 3. 描述符集在哪里被使用？

### 3.1 创建阶段（ShadingMaterial::DoCreate）

```cpp
bool ShadingMaterial::DoCreate(RHI::IDevice* device) {
    // ... 创建 shader modules ...
    
    // 创建 uniform buffers
    CreateUniformBuffers(device);
    
    // 创建描述符集（收集绑定信息）
    CreateDescriptorSets(device);
    
    // 注意：实际的 GPU 描述符集对象创建是延迟的
    // 需要在设备实现层完成
}
```

### 3.2 获取阶段（SceneRenderer::SubmitRenderQueue）

```cpp
void SceneRenderer::SubmitRenderQueue(...) {
    for (const auto& item : items) {
        // 从 ShadingMaterial 获取描述符集
        void* itemDescriptorSet = nullptr;
        
        if (item.materialData.shadingMaterial) {
            auto* shadingMaterial = static_cast<ShadingMaterial*>(...);
            itemDescriptorSet = shadingMaterial->GetDescriptorSet(0); // 获取 Set 0
        }
        
        // 绑定描述符集到渲染命令
        if (itemDescriptorSet != currentDescriptorSet) {
            RenderCommand cmd;
            cmd.type = RenderCommandType::BindDescriptorSets;
            cmd.params.bindDescriptorSets.descriptorSets = {itemDescriptorSet};
            commandList.AddCommand(std::move(cmd));
        }
    }
}
```

### 3.3 批处理阶段（RenderBatch）

```cpp
// 描述符集用于排序和批处理
bool RenderBatch::ShouldBatch(const RenderItem& a, const RenderItem& b) {
    // 如果描述符集不同，不能批处理
    if (a.materialData.descriptorSet != b.materialData.descriptorSet) {
        return false;
    }
    // ... 其他检查 ...
}
```

### 3.4 命令记录阶段（CommandRecorder）

```cpp
void CommandRecorder::RecordBindDescriptorSets(...) {
    if (!params.descriptorSets.empty()) {
        // 将描述符集绑定到命令缓冲区
        cmd->BindDescriptorSets(
            params.firstSet,        // 起始 set 索引（通常是 0）
            params.descriptorSets,  // 描述符集列表
            params.dynamicOffsets   // 动态偏移量（用于动态 uniform buffer）
        );
    }
}
```

### 3.5 GPU 绑定阶段（VulkanCommandBuffer）

```cpp
void VulkanCommandBuffer::BindDescriptorSets(...) {
    // TODO: 实现实际的 Vulkan 描述符集绑定
    // 应该调用 vkCmdBindDescriptorSets
    // vkCmdBindDescriptorSets(
    //     m_VkCommandBuffer,
    //     VK_PIPELINE_BIND_POINT_GRAPHICS,
    //     pipelineLayout,
    //     firstSet,
    //     descriptorSets.size(),
    //     descriptorSets.data(),
    //     dynamicOffsets.size(),
    //     dynamicOffsets.data()
    // );
}
```

## 4. 在渲染过程中的作用

### 4.1 渲染流程中的位置

```
1. 准备阶段（每帧开始）
   ├─ 收集渲染参数（RenderParameterCollector）
   ├─ 更新 ShadingMaterial 参数（UpdateRenderParameters）
   └─ 刷新到 GPU（FlushParametersToGPU）
       └─ 更新 Uniform Buffer 数据（DoUpdate）

2. 构建渲染队列（SceneRenderer::BuildRenderQueue）
   ├─ 遍历 Entity
   ├─ 创建 RenderItem
   └─ 获取描述符集（GetDescriptorSet）

3. 提交渲染队列（SceneRenderer::SubmitRenderQueue）
   ├─ 转换为 RenderCommand
   ├─ 绑定 Pipeline
   ├─ 绑定 Descriptor Sets ← 这里！
   ├─ 绑定 Vertex Buffers
   ├─ 绑定 Index Buffer
   └─ Draw Call

4. 记录命令（CommandRecorder）
   └─ 将 BindDescriptorSets 命令记录到命令缓冲区

5. GPU 执行（VulkanCommandBuffer）
   └─ 实际绑定描述符集到 GPU
```

### 4.2 描述符集的作用

#### A. 资源组织
- **统一管理**：将相关的 uniform buffer 和 texture 组织在一起
- **按需绑定**：可以只绑定需要的描述符集，而不是所有资源

#### B. 性能优化
- **批处理**：相同描述符集的渲染项可以批处理，减少状态切换
- **缓存友好**：描述符集可以被 GPU 缓存，提高访问效率

#### C. 灵活性
- **多 Set 设计**：可以使用不同的 set 组织不同类型的资源
  - Set 0：Per-Object 数据（每个对象不同）
  - Set 1：Per-Frame 数据（每帧更新一次）
  - Set 2：Per-Material 数据（每个材质不同）

#### D. Shader 访问
- **统一接口**：Shader 通过 `set` 和 `binding` 访问资源
- **类型安全**：描述符集布局确保资源类型匹配

### 4.3 实际渲染示例

假设有一个 Shader：

```hlsl
// Set 0: Per-Object 数据
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
} ubo;

layout(set = 0, binding = 1) uniform texture2D albedoMap;

// Set 1: Per-Frame 数据
layout(set = 1, binding = 0) uniform PerFrameData {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} perFrame;
```

渲染流程：

```cpp
// 1. 创建描述符集（在 ShadingMaterial::DoCreate）
// Set 0: {binding 0: ubo buffer, binding 1: albedoMap texture}
// Set 1: {binding 0: perFrame buffer} (可能由全局渲染器管理)

// 2. 每帧更新数据
shadingMaterial->UpdateUniformBufferByName("UniformBufferObject", &modelMatrix, sizeof(mat4));
shadingMaterial->UpdateTextureByName("albedoMap", texture);

// 3. 刷新到 GPU
shadingMaterial->FlushParametersToGPU(device);

// 4. 渲染时绑定
cmd->BindDescriptorSets(0, {set0_descriptorSet, set1_descriptorSet}, {});

// 5. GPU 执行 Draw Call
cmd->DrawIndexed(indexCount, ...);
```

## 5. 当前实现的限制

### 5.1 占位符实现
- `CreateDescriptorSets` 目前只是收集信息，不创建实际的 GPU 对象
- `VulkanCommandBuffer::BindDescriptorSets` 是 TODO，未实现

### 5.2 需要完善的部分

1. **描述符集布局创建**（Descriptor Set Layout）
   ```cpp
   // 需要根据 setBindings 创建 VkDescriptorSetLayout
   VkDescriptorSetLayoutCreateInfo layoutInfo = {...};
   vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
   ```

2. **描述符池创建**（Descriptor Pool）
   ```cpp
   // 需要创建描述符池来分配描述符集
   VkDescriptorPoolCreateInfo poolInfo = {...};
   vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
   ```

3. **描述符集分配**（Descriptor Set Allocation）
   ```cpp
   // 从池中分配描述符集
   VkDescriptorSetAllocateInfo allocInfo = {...};
   vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
   ```

4. **描述符写入**（Descriptor Write）
   ```cpp
   // 将实际的 buffer 和 texture 写入描述符集
   VkWriteDescriptorSet write = {...};
   write.pBufferInfo = &bufferInfo;  // Uniform buffer 信息
   write.pImageInfo = &imageInfo;    // Texture 信息
   vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
   ```

5. **命令绑定**（Command Binding）
   ```cpp
   // 在命令缓冲区中绑定描述符集
   vkCmdBindDescriptorSets(
       commandBuffer,
       VK_PIPELINE_BIND_POINT_GRAPHICS,
       pipelineLayout,
       firstSet,
       descriptorSets.size(),
       descriptorSets.data(),
       dynamicOffsets.size(),
       dynamicOffsets.data()
   );
   ```

## 6. 总结

### 关键点
1. **描述符集是资源的容器**：组织 uniform buffer 和 texture
2. **数据来自 Shader Reflection**：从编译后的 shader 中解析
3. **在渲染前绑定**：通过命令缓冲区绑定到 GPU
4. **用于批处理和优化**：相同描述符集的渲染项可以批处理

### 当前状态
- ✅ 数据结构已准备（setBindings）
- ✅ 接口已定义（GetDescriptorSet/SetDescriptorSet）
- ✅ 渲染流程已集成（SceneRenderer）
- ⚠️ GPU 对象创建未实现（需要完善 VulkanDevice）
- ⚠️ 命令绑定未实现（需要完善 VulkanCommandBuffer）

### 下一步
需要实现完整的 Vulkan 描述符集创建和绑定流程，才能让描述符集真正发挥作用。
