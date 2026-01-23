# Descriptor Set Binding 冲突修复 - 快速总结

## 问题速览

| 问题 | 错误信息 | 解决方案 |
|------|----------|----------|
| 描述符类型不匹配 | `VUID-VkGraphicsPipelineCreateInfo-layout-07990` | 增强着色器反射，正确识别纹理资源类型 |
| 纹理资源未识别 | `sampled_images count: 0` | 优先使用片段着色器进行反射，添加回退逻辑 |
| Binding 冲突 | `Binding conflict detected! Set 0, Binding 0` | 改进冲突检测，创建 HLSL 指南 |
| 管线未绑定 | `VUID-vkCmdDrawIndexed-None-08606` | 添加管线绑定验证和 null 检查 |

## 核心修改

### 1. 着色器反射增强
- **文件**: `include/FirstEngine/Shader/ShaderCompiler.h`, `src/Shader/ShaderCompiler.cpp`
- **改动**: 添加 `sampled_images`、`separate_images`、`separate_samplers` 字段
- **影响**: 能够区分 Combined Image Sampler、Separate Image、Separate Sampler

### 2. 描述符类型映射
- **文件**: `include/FirstEngine/Renderer/ShadingMaterial.h`, `src/Renderer/ShadingMaterial.cpp`
- **改动**: 在 `TextureBinding` 中添加 `descriptorType` 字段
- **影响**: 正确设置每个纹理绑定的描述符类型

### 3. Binding 冲突检测
- **文件**: `src/Renderer/MaterialDescriptorManager.cpp`
- **改动**: 改进冲突检测逻辑，允许 separate sampler/image 配对
- **影响**: 提供清晰的错误信息，防止无效的 binding 分配

### 4. 管线绑定验证
- **文件**: `src/Renderer/SceneRenderer.cpp`, `src/Renderer/CommandRecorder.cpp`
- **改动**: 添加 null 检查和绑定验证
- **影响**: 确保绘制命令执行前管线已正确绑定

## HLSL Binding 分配规则

### ✅ 正确示例
```hlsl
// Uniform Buffers
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(1, 0)]] cbuffer LightingParams { ... };

// Separate Images
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] Texture2D aoMap;

// Separate Sampler
[[vk::binding(6, 0)]] SamplerState textureSampler;
```

### ❌ 错误示例
```hlsl
// 错误：Uniform buffer 和 Texture 使用相同的 binding
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(0, 0)]] Texture2D albedoMap;  // ❌ 冲突！
```

## 关键规则

1. **同一 descriptor set 中，每个 binding 只能有一种资源类型**
2. **Uniform buffers 和 textures 不能共享 binding**
3. **Separate images 和 separate samplers 需要不同的 binding**

## 相关文档

- 详细文档: `docs/BugFixes/DescriptorSet_Binding_Conflicts_Fix.md`
- HLSL 指南: `docs/HLSL_Binding_Guide.md`
