# Descriptor Types 详解：Sampler、Image 和 CombinedImageSampler

## 概述

在 Vulkan 和现代图形 API 中，有三种不同的方式来处理纹理采样：

1. **CombinedImageSampler** - 图像和采样器绑定在一起
2. **Separate Image + Separate Sampler** - 图像和采样器分开绑定
3. **Sampler** - 只有采样器（用于与单独的图像配合使用）

## 1. CombinedImageSampler（组合图像采样器）

### 定义
- **Vulkan 类型**: `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`
- **描述**: 图像（ImageView）和采样器（Sampler）组合在一起作为一个描述符
- **用途**: 传统 GLSL 风格的 `sampler2D`，图像和采样器不可分离

### Shader 语法（GLSL）
```glsl
layout(set = 0, binding = 0) uniform sampler2D myTexture;
```

### Shader 语法（HLSL with Vulkan）
```hlsl
// HLSL 不支持原生的 CombinedImageSampler
// 需要使用 separate image + separate sampler
```

### Binding 要求
- **Descriptor Set Layout**: 需要 `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`
- **Descriptor Write**: 需要同时提供 `imageView` 和 `sampler`（两者都不能为 `VK_NULL_HANDLE`）
- **使用**: `texture(myTexture, uv)` - 直接采样

### 优点
- 简单易用，图像和采样器绑定在一起
- 兼容传统 GLSL 代码

### 缺点
- 不能在不同纹理之间共享采样器
- 灵活性较低

---

## 2. Separate Image（分离图像）

### 定义
- **Vulkan 类型**: `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`
- **描述**: 只有图像（ImageView），没有采样器
- **用途**: 需要与单独的采样器配合使用

### Shader 语法（HLSL with Vulkan）
```hlsl
[[vk::binding(2, 0)]] Texture2D albedoMap;  // Separate Image
[[vk::binding(6, 0)]] SamplerState textureSampler;  // Separate Sampler
```

### Binding 要求
- **Descriptor Set Layout**: 需要 `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE`
- **Descriptor Write**: 只需要 `imageView`（不能为 `VK_NULL_HANDLE`），`sampler` 必须为 `VK_NULL_HANDLE`
- **使用**: `albedoMap.Sample(textureSampler, uv)` - 使用单独的采样器采样

### 优点
- 可以在多个纹理之间共享同一个采样器
- 更灵活，可以动态组合图像和采样器

### 缺点
- 需要两个绑定（图像 + 采样器）
- 代码稍微复杂一些

---

## 3. Separate Sampler（分离采样器）

### 定义
- **Vulkan 类型**: `VK_DESCRIPTOR_TYPE_SAMPLER`
- **描述**: 只有采样器（Sampler），没有图像
- **用途**: 与单独的图像配合使用，可以在多个纹理之间共享

### Shader 语法（HLSL with Vulkan）
```hlsl
[[vk::binding(6, 0)]] SamplerState textureSampler;  // Separate Sampler
```

### Binding 要求
- **Descriptor Set Layout**: 需要 `VK_DESCRIPTOR_TYPE_SAMPLER`
- **Descriptor Write**: 只需要 `sampler`（不能为 `VK_NULL_HANDLE`），`imageView` 必须为 `VK_NULL_HANDLE`
- **使用**: 与 `Texture2D` 配合使用，如 `albedoMap.Sample(textureSampler, uv)`

### 优点
- 可以在多个纹理之间共享同一个采样器
- 节省描述符资源（多个纹理共享一个采样器）

### 缺点
- 需要两个绑定（图像 + 采样器）
- 代码稍微复杂一些

---

## 当前实现的问题

### 问题描述
当前代码中，`separate sampler` 被当作 `CombinedImageSampler` 处理（作为 fallback），但：
1. `CombinedImageSampler` 要求 `imageView` 不能为 `VK_NULL_HANDLE`
2. `separate sampler` 不需要 `imageView`，只需要 `sampler`
3. 这导致了 validation 错误

### 当前代码的问题位置

**MaterialDescriptorManager.cpp** (第 456-483 行):
```cpp
// 对于 separate sampler (texture == nullptr)
// 使用 CombinedImageSampler 作为 fallback
RHI::DescriptorImageInfo samplerInfo;
samplerInfo.image = nullptr;
samplerInfo.imageView = nullptr; // VK_NULL_HANDLE - 这会导致错误！
samplerInfo.sampler = vkDevice->GetDefaultSampler();
```

**VulkanDevice.cpp** (第 1205-1208 行):
```cpp
if (!imgInfo.image) {
    std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Image is nullptr for binding " 
              << write.dstBinding << std::endl;
    vkImgInfo.imageView = VK_NULL_HANDLE;  // 这会导致 validation 错误
}
```

### 解决方案

需要：
1. 在 RHI 层添加 `DescriptorType::Sampler` 支持
2. 在 Vulkan 层正确处理 `VK_DESCRIPTOR_TYPE_SAMPLER`
3. 在 MaterialDescriptorManager 中，对于 separate sampler，使用正确的类型

---

## 三种类型的对比表

| 类型 | Vulkan 类型 | 需要 ImageView | 需要 Sampler | Shader 使用 | 灵活性 |
|------|------------|----------------|--------------|------------|--------|
| **CombinedImageSampler** | `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` | ✅ 必须 | ✅ 必须 | `texture(sampler2D, uv)` | 低 |
| **SampledImage** | `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE` | ✅ 必须 | ❌ 不需要 | `texture2D.Sample(sampler, uv)` | 中 |
| **Sampler** | `VK_DESCRIPTOR_TYPE_SAMPLER` | ❌ 不需要 | ✅ 必须 | `texture2D.Sample(sampler, uv)` | 高 |

---

## 实际使用示例

### 示例 1: CombinedImageSampler（GLSL）
```glsl
// Shader
layout(set = 0, binding = 0) uniform sampler2D myTexture;

// 使用
vec4 color = texture(myTexture, uv);
```

### 示例 2: Separate Image + Sampler（HLSL/Vulkan）
```hlsl
// Shader
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(6, 0)]] SamplerState textureSampler;

// 使用
float4 color = albedoMap.Sample(textureSampler, uv);
```

### 示例 3: 多个纹理共享一个采样器
```hlsl
// Shader
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(6, 0)]] SamplerState textureSampler;  // 所有纹理共享这个采样器

// 使用
float4 albedo = albedoMap.Sample(textureSampler, uv);
float4 normal = normalMap.Sample(textureSampler, uv);
float4 metallicRoughness = metallicRoughnessMap.Sample(textureSampler, uv);
```

---

## 修复建议

1. **添加 RHI::DescriptorType::Sampler 支持**
2. **在 VulkanDevice 中正确处理 VK_DESCRIPTOR_TYPE_SAMPLER**
3. **在 MaterialDescriptorManager 中，对于 separate sampler，使用正确的类型而不是 fallback**
