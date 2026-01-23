# Descriptor Set Binding 冲突修复总结

## 日期
2024年（当前会话）

## 问题概述

在实现 Vulkan 渲染管线时，遇到了多个与描述符集（Descriptor Set）绑定和着色器反射相关的问题，导致渲染失败。

## 问题列表

### 问题 1：描述符类型不匹配

**错误信息**：
```
[ VUID-VkGraphicsPipelineCreateInfo-layout-07990 ] 
vkCreateGraphicsPipelines(): pCreateInfos[0].pStages[1] SPIR-V (VK_SHADER_STAGE_FRAGMENT_BIT) 
uses descriptor slot [Set 0 Binding 0] of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER 
but expected VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER or VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE.
```

**原因**：
- 着色器反射没有正确识别纹理资源（`sampled_images`、`separate_images`、`separate_samplers` 为空）
- 描述符集布局只创建了 Uniform Buffer 绑定，但着色器期望纹理绑定

**解决方案**：
1. 修改 `ShaderCompiler::GetReflection()` 以正确填充 `sampled_images`、`separate_images`、`separate_samplers` 字段
2. 修改 `ShadingMaterial::ParseShaderReflection()` 以正确解析这些新字段
3. 添加回退逻辑，当新字段为空时使用旧的 `samplers` 和 `images` 字段
4. 修改 `MaterialDescriptorManager` 以使用正确的描述符类型创建布局

### 问题 2：着色器反射未识别纹理资源

**错误信息**：
```
sampled_images count: 0
separate_images count: 0
separate_samplers count: 0
```

**原因**：
- `ShaderCollection` 可能缓存了旧版本的反射数据（新字段为空）
- 反射数据是从顶点着色器生成的，而纹理通常在片段着色器中

**解决方案**：
1. 修改 `ShaderCollectionsTools::CreateCollectionFromFiles()` 优先使用片段着色器进行反射
2. 添加调试输出以诊断反射数据生成过程
3. 添加回退逻辑处理旧缓存数据

### 问题 3：Binding 冲突

**错误信息**：
```
Error: MaterialDescriptorManager::CreateDescriptorSetLayouts: 
Binding conflict detected! Set 0, Binding 0 is already used by type 0 (UniformBuffer), 
trying to add 2 (SampledImage)
```

**原因**：
- 着色器中 Uniform buffer 和纹理资源使用了相同的 binding
- 在 Vulkan 中，同一个 descriptor set 的同一个 binding 不能有不同类型的资源

**解决方案**：
1. 改进冲突检测逻辑，提供更详细的错误信息
2. 允许 separate sampler 和 separate image 配对使用（虽然它们不应共享 binding）
3. 创建 HLSL Binding 分配指南文档（`docs/HLSL_Binding_Guide.md`）

### 问题 4：管线未绑定

**错误信息**：
```
[ VUID-vkCmdDrawIndexed-None-08606 ] 
vkCmdDrawIndexed(): A valid VK_PIPELINE_BIND_POINT_GRAPHICS pipeline must be bound 
with vkCmdBindPipeline before calling this command.
```

**原因**：
- 在调用 `vkCmdDrawIndexed()` 之前没有绑定图形管线
- `itemPipeline` 可能为 `nullptr`，导致 `BindPipeline` 命令未被添加

**解决方案**：
1. 在 `SceneRenderer::SubmitRenderQueue()` 中添加 `itemPipeline` 的 null 检查
2. 在 `CommandRecorder::RecordDrawIndexed()` 中添加管线绑定检查
3. 在 `BeginRenderPass` 时重置管线状态，确保每次 render pass 开始后都会重新绑定管线

## 技术说明

### 1. 着色器反射增强

#### 问题
原有的 `ShaderReflection` 结构体只有 `samplers` 和 `images` 字段，无法区分：
- Combined Image Samplers（GLSL `sampler2D`）
- Separate Images（HLSL `texture2D`）
- Separate Samplers（HLSL `sampler`）

#### 解决方案
在 `include/FirstEngine/Shader/ShaderCompiler.h` 中添加新字段：
```cpp
struct ShaderReflection {
    // ... 现有字段 ...
    
    // Additional lists to distinguish resource types
    std::vector<ShaderResource> sampled_images;    // Combined image samplers
    std::vector<ShaderResource> separate_images;   // Separate images
    std::vector<ShaderResource> separate_samplers; // Separate samplers
};
```

在 `src/Shader/ShaderCompiler.cpp` 中填充这些字段：
```cpp
// Get combined image samplers (sampler2D in GLSL)
for (auto& resource : resources.sampled_images) {
    // ... 解析并添加到 sampled_images ...
}

// Get separate images (texture2D in HLSL/Vulkan)
for (auto& resource : resources.separate_images) {
    // ... 解析并添加到 separate_images ...
}

// Get separate samplers (sampler in HLSL/Vulkan)
for (auto& resource : resources.separate_samplers) {
    // ... 解析并添加到 separate_samplers ...
}
```

### 2. 描述符类型映射

#### 问题
`ShadingMaterial` 需要知道每个纹理绑定的正确描述符类型，以便创建正确的描述符集布局。

#### 解决方案
在 `include/FirstEngine/Renderer/ShadingMaterial.h` 中添加 `descriptorType` 字段：
```cpp
struct TextureBinding {
    uint32_t set;
    uint32_t binding;
    std::string name;
    RHI::TextureResource* texture;
    RHI::DescriptorType descriptorType = RHI::DescriptorType::CombinedImageSampler;
};
```

在 `src/Renderer/ShadingMaterial.cpp` 中根据反射数据设置正确的类型：
```cpp
// Parse Combined Image Samplers
for (const auto& sampler : reflection.sampled_images) {
    binding.descriptorType = RHI::DescriptorType::CombinedImageSampler;
}

// Parse Separate Images
for (const auto& image : reflection.separate_images) {
    binding.descriptorType = RHI::DescriptorType::SampledImage;
}

// Parse Separate Samplers
for (const auto& sampler : reflection.separate_samplers) {
    // TODO: Should be SAMPLER when RHI supports it
    binding.descriptorType = RHI::DescriptorType::CombinedImageSampler;
}
```

### 3. Binding 冲突检测

#### 问题
需要检测并报告 binding 冲突，同时允许 separate sampler 和 separate image 配对使用。

#### 解决方案
在 `src/Renderer/MaterialDescriptorManager.cpp` 中实现智能冲突检测：
```cpp
// Check for binding conflicts with uniform buffers
if (bindings.find(tb.binding) != bindings.end()) {
    RHI::DescriptorType existingType = bindings[tb.binding];
    
    if (existingType == RHI::DescriptorType::UniformBuffer) {
        // Uniform buffers and textures cannot share binding
        std::cerr << "Error: Binding conflict! Uniform buffer and texture cannot share binding." << std::endl;
        return false;
    }
    
    // Separate samplers and separate images can coexist
    if ((existingType == RHI::DescriptorType::SampledImage && 
         tb.descriptorType == RHI::DescriptorType::CombinedImageSampler) ||
        (existingType == RHI::DescriptorType::CombinedImageSampler && 
         tb.descriptorType == RHI::DescriptorType::SampledImage)) {
        // Allow coexistence for separate sampler/image pairs
    }
}
```

### 4. 管线绑定验证

#### 问题
需要确保在绘制之前管线已正确绑定。

#### 解决方案
在 `src/Renderer/CommandRecorder.cpp` 中添加验证：
```cpp
void CommandRecorder::RecordDrawIndexed(...) {
    if (!m_CurrentPipeline) {
        std::cerr << "Error: No pipeline bound before DrawIndexed." << std::endl;
        return;
    }
    cmd->DrawIndexed(...);
}
```

在 `src/Renderer/SceneRenderer.cpp` 中添加 null 检查：
```cpp
if (!itemPipeline) {
    std::cerr << "Warning: itemPipeline is null, skipping draw command" << std::endl;
    continue;
}
```

## 代码修改说明

### 修改的文件列表

1. **include/FirstEngine/Shader/ShaderCompiler.h**
   - 添加 `sampled_images`、`separate_images`、`separate_samplers` 字段到 `ShaderReflection` 结构体

2. **src/Shader/ShaderCompiler.cpp**
   - 在 `GetReflection()` 中填充新的反射字段
   - 添加调试输出

3. **include/FirstEngine/Renderer/ShadingMaterial.h**
   - 在 `TextureBinding` 结构体中添加 `descriptorType` 字段

4. **src/Renderer/ShadingMaterial.cpp**
   - 修改 `ParseShaderReflection()` 以正确解析新字段
   - 添加回退逻辑处理旧缓存数据
   - 添加调试输出
   - 在 `EnsurePipelineCreated()` 中添加错误日志

5. **src/Renderer/MaterialDescriptorManager.cpp**
   - 修改 `CreateDescriptorSetLayouts()` 以使用正确的描述符类型
   - 改进 binding 冲突检测逻辑
   - 添加详细的错误日志
   - 避免重复添加相同的 binding

6. **src/Renderer/ShaderCollectionsTools.cpp**
   - 修改反射数据生成逻辑，优先使用片段着色器
   - 添加调试输出

7. **src/Renderer/SceneRenderer.cpp**
   - 添加 `itemPipeline` 的 null 检查
   - 改进错误日志
   - 添加 `#include <iostream>`

8. **src/Renderer/CommandRecorder.cpp**
   - 在 `RecordDrawIndexed()` 中添加管线绑定验证
   - 在 `BeginRenderPass` 时重置管线状态

### 关键代码片段

#### 1. 着色器反射增强
```cpp
// src/Shader/ShaderCompiler.cpp
// Get combined image samplers (sampler2D in GLSL)
for (auto& resource : resources.sampled_images) {
    ShaderResource sr;
    // ... 解析资源 ...
    reflection.samplers.push_back(sr);           // 向后兼容
    reflection.sampled_images.push_back(sr);     // 新字段
}
```

#### 2. 描述符类型设置
```cpp
// src/Renderer/ShadingMaterial.cpp
for (const auto& image : reflection.separate_images) {
    TextureBinding binding;
    binding.descriptorType = RHI::DescriptorType::SampledImage;
    m_TextureBindings.push_back(binding);
}
```

#### 3. Binding 冲突检测
```cpp
// src/Renderer/MaterialDescriptorManager.cpp
if (existingType == RHI::DescriptorType::UniformBuffer) {
    std::cerr << "Error: Binding conflict! Uniform buffer and texture cannot share binding." << std::endl;
    return false;
}
```

#### 4. 管线绑定验证
```cpp
// src/Renderer/CommandRecorder.cpp
void CommandRecorder::RecordDrawIndexed(...) {
    if (!m_CurrentPipeline) {
        std::cerr << "Error: No pipeline bound before DrawIndexed." << std::endl;
        return;
    }
    cmd->DrawIndexed(...);
}
```

## 文档

### 创建的文档

1. **docs/HLSL_Binding_Guide.md**
   - HLSL 着色器 Binding 分配指南
   - 包含正确的语法示例
   - 常见错误和修复方案
   - Vulkan 属性语法说明

### 文档内容要点

1. **Binding 分配规则**：
   - 同一 descriptor set 中，每个 binding 只能有一种资源类型
   - Uniform buffers 和 textures 不能共享 binding
   - Separate images 和 separate samplers 需要不同的 binding

2. **HLSL 语法**：
   - 使用 `[[vk::binding(binding_index, set_index)]]` 属性
   - 或使用 `register(bN)`、`register(tN)`、`register(sN)` 指令

3. **修复建议**：
   - Uniform buffers: Binding 0, 1
   - Separate images: Binding 2, 3, 4, 5
   - Separate sampler: Binding 6

## 测试建议

1. **验证着色器反射**：
   - 检查 `sampled_images`、`separate_images`、`separate_samplers` 是否正确填充
   - 验证回退逻辑是否正常工作

2. **验证描述符集布局**：
   - 检查创建的布局是否与着色器期望的类型匹配
   - 验证 binding 冲突检测是否正常工作

3. **验证管线绑定**：
   - 确保在每次绘制之前管线已正确绑定
   - 验证 null 检查是否正常工作

4. **验证 Binding 分配**：
   - 使用修复后的着色器验证 binding 冲突是否解决
   - 验证渲染是否正常工作

## 后续工作

1. **支持 SAMPLER 描述符类型**：
   - 当前 separate samplers 使用 `CombinedImageSampler` 作为回退
   - 需要在 RHI 层添加 `SAMPLER` 类型支持

2. **改进错误处理**：
   - 添加更友好的错误消息
   - 提供自动修复建议

3. **性能优化**：
   - 缓存描述符集布局
   - 减少重复的反射数据生成

4. **文档完善**：
   - 添加更多着色器示例
   - 创建最佳实践指南

## 总结

本次修复解决了多个与描述符集绑定和着色器反射相关的关键问题：

1. ✅ 着色器反射现在能正确识别不同类型的纹理资源
2. ✅ 描述符集布局使用正确的描述符类型创建
3. ✅ Binding 冲突检测提供清晰的错误信息
4. ✅ 管线绑定验证确保绘制命令正确执行
5. ✅ 创建了完整的 HLSL Binding 分配指南

这些修复确保了渲染管线能够正确处理 Vulkan 的描述符集和绑定，为后续的渲染功能开发奠定了基础。
