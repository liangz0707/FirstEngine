# 着色器 Binding 修复总结

## 修复日期
2024年（当前会话）

## 修复概述

修复了 Package/Shaders 目录下所有 HLSL 着色器文件的 binding 冲突问题，统一使用 Vulkan binding 语法（`[[vk::binding()]]`），移除了可能导致冲突的 `register()` 指令。

## 修复的文件列表

### 1. PBR 着色器
- **PBR.frag.hlsl**: 修复了 Uniform buffer 和 Texture 的 binding 冲突
- **PBR.vert.hlsl**: 添加了 Vulkan binding 语法

### 2. SunLight 着色器
- **SunLight.frag.hlsl**: 修复了所有资源的 binding 分配
- **SunLight.vert.hlsl**: 添加了 Vulkan binding 语法

### 3. IBL 着色器
- **IBL.frag.hlsl**: 修复了多个纹理和采样器的 binding 分配
- **IBL.vert.hlsl**: 添加了 Vulkan binding 语法

### 4. Light 着色器
- **Light.frag.hlsl**: 修复了 G-Buffer 纹理的 binding 分配
- **Light.vert.hlsl**: 添加了 Vulkan binding 语法

### 5. PostProcess 着色器
- **PostProcess.frag.hlsl**: 修复了纹理和采样器的 binding 分配
- **PostProcess.vert.hlsl**: 无需修改（无资源绑定）

### 6. Tonemapping 着色器
- **Tonemapping.frag.hlsl**: 修复了纹理和采样器的 binding 分配
- **Tonemapping.vert.hlsl**: 无需修改（无资源绑定）

### 7. GeometryShader 着色器
- **GeometryShader.frag.hlsl**: 修复了多个纹理的 binding 分配
- **GeometryShader.vert.hlsl**: 添加了 Vulkan binding 语法

## Binding 分配策略

### 统一规则
1. **Set 0**: MaterialParams/LightParams 和纹理（片段着色器使用）
2. **Set 1**: PerObject 和 PerFrame（顶点着色器使用，也可以被片段着色器使用）
3. **Uniform Buffers**: 从 Binding 0 开始
4. **Separate Images**: 在 Uniform Buffers 之后
5. **Separate Samplers**: 在 Images 之后

### 重要说明
- **PerObject 和 PerFrame 统一在 Set 1**：避免与片段着色器的 MaterialParams 等资源冲突
- **顶点着色器**：使用 Set 1 的 PerObject (Binding 0) 和 PerFrame (Binding 1)
- **片段着色器**：使用 Set 0 的 MaterialParams/LightParams 和纹理，如果需要 PerFrame 则使用 Set 1

### 各着色器的 Binding 分配

#### PBR.frag.hlsl
```
Binding 0: MaterialParams (Uniform Buffer)
Binding 1: LightingParams (Uniform Buffer)
Binding 2: albedoMap (Separate Image)
Binding 3: normalMap (Separate Image)
Binding 4: metallicRoughnessMap (Separate Image)
Binding 5: aoMap (Separate Image)
Binding 6: textureSampler (Separate Sampler)
```

#### SunLight.frag.hlsl
```
Binding 0: MaterialParams (Uniform Buffer)
Binding 1: SunLightParams (Uniform Buffer)
Binding 2: albedoMap (Separate Image)
Binding 3: normalMap (Separate Image)
Binding 4: metallicRoughnessMap (Separate Image)
Binding 5: shadowMap (Separate Image)
Binding 6: textureSampler (Separate Sampler)
Binding 7: shadowSampler (Separate Sampler)
```

#### IBL.frag.hlsl
```
Binding 0: MaterialParams (Uniform Buffer)
Binding 1: IBLParams (Uniform Buffer)
Binding 2: albedoMap (Separate Image)
Binding 3: normalMap (Separate Image)
Binding 4: metallicRoughnessMap (Separate Image)
Binding 5: irradianceMap (Separate Image - Cube)
Binding 6: prefilterMap (Separate Image - Cube)
Binding 7: brdfLUT (Separate Image)
Binding 8: textureSampler (Separate Sampler)
Binding 9: cubeSampler (Separate Sampler)
```

#### Light.frag.hlsl
```
Set 0:
  Binding 0: LightParams (Uniform Buffer)
  Binding 1: gAlbedo (Separate Image - G-Buffer)
  Binding 2: gNormal (Separate Image - G-Buffer)
  Binding 3: gMaterial (Separate Image - G-Buffer)
  Binding 4: gDepth (Separate Image - G-Buffer)
  Binding 5: gBufferSampler (Separate Sampler)
Set 1:
  Binding 1: PerFrame (Uniform Buffer) - matches vertex shaders
```

#### PostProcess.frag.hlsl
```
Binding 0: PostProcessParams (Uniform Buffer)
Binding 1: inputTexture (Separate Image)
Binding 2: textureSampler (Separate Sampler)
```

#### Tonemapping.frag.hlsl
```
Binding 0: TonemappingParams (Uniform Buffer)
Binding 1: hdrTexture (Separate Image)
Binding 2: textureSampler (Separate Sampler)
```

#### GeometryShader.frag.hlsl
```
Binding 0: MaterialParams (Uniform Buffer)
Binding 1: albedoMap (Separate Image)
Binding 2: normalMap (Separate Image)
Binding 3: metallicRoughnessMap (Separate Image)
Binding 4: aoMap (Separate Image)
Binding 5: emissionMap (Separate Image)
Binding 6: textureSampler (Separate Sampler)
```

## 主要修改

### 1. 移除 register 指令
**之前**:
```hlsl
Texture2D albedoMap : register(t0);
cbuffer MaterialParams : register(b0) { ... };
```

**之后**:
```hlsl
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(2, 0)]] Texture2D albedoMap;
```

### 2. 统一使用 Vulkan binding 语法
所有资源现在都使用 `[[vk::binding(binding_index, set_index)]]` 语法，确保：
- 明确的 binding 分配
- 避免与 register 指令的冲突
- 更好的 Vulkan 兼容性

### 3. 确保 binding 不重复
- 每个资源都有唯一的 binding
- Uniform buffers 和 textures 使用不同的 binding
- Separate images 和 separate samplers 使用不同的 binding

## 验证

修复后，所有着色器应该：
1. ✅ 没有 binding 冲突
2. ✅ 使用统一的 Vulkan binding 语法
3. ✅ 与描述符集布局创建逻辑兼容
4. ✅ 能够正确通过 Vulkan 验证层

## 注意事项

1. **重新编译着色器**: 修复后需要重新编译所有着色器文件为 SPIR-V
2. **清除缓存**: 如果使用了着色器缓存，可能需要清除缓存以使用新的反射数据
3. **测试**: 建议测试每个着色器以确保渲染正常工作

## 相关文档

- HLSL Binding 指南: `docs/HLSL_Binding_Guide.md`
- Binding 冲突修复总结: `docs/BugFixes/DescriptorSet_Binding_Conflicts_Fix.md`
