# HLSL 着色器 Binding 分配指南

## 问题说明

在 Vulkan 中，同一个 descriptor set 的同一个 binding 不能有不同类型的资源。例如：
- ❌ **错误**：Uniform buffer 在 Binding 0，Texture 也在 Binding 0
- ✅ **正确**：Uniform buffer 在 Binding 0，Texture 在 Binding 1

## HLSL 语法

### 方法 1：使用 Vulkan 属性（推荐）

```hlsl
// 顶点着色器示例
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

// Uniform Buffer - Set 0, Binding 0
cbuffer MaterialParams : register(b0) {
    float4 baseColor;
    float metallic;
    float roughness;
    float padding;
};

// Uniform Buffer - Set 0, Binding 1
cbuffer LightingParams : register(b1) {
    float4 lightDirection;
    float4 lightColor;
};

// 或者使用 Vulkan 属性（更明确）
[[vk::binding(0, 0)]] cbuffer MaterialParams {
    float4 baseColor;
    float metallic;
    float roughness;
    float padding;
};

[[vk::binding(1, 0)]] cbuffer LightingParams {
    float4 lightDirection;
    float4 lightColor;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    output.texCoord = input.texCoord;
    return output;
}
```

```hlsl
// 片段着色器示例
struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

// Uniform Buffers - Set 0, Binding 0 和 1
[[vk::binding(0, 0)]] cbuffer MaterialParams {
    float4 baseColor;
    float metallic;
    float roughness;
    float padding;
};

[[vk::binding(1, 0)]] cbuffer LightingParams {
    float4 lightDirection;
    float4 lightColor;
};

// Separate Images - Set 0, Binding 2, 3, 4, 5
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] Texture2D aoMap;

// Separate Sampler - Set 0, Binding 6
[[vk::binding(6, 0)]] SamplerState textureSampler;

float4 PSMain(PSInput input) : SV_TARGET {
    // 使用 separate image 和 sampler
    float4 albedo = albedoMap.Sample(textureSampler, input.texCoord);
    float3 normal = normalMap.Sample(textureSampler, input.texCoord).rgb;
    float2 metallicRoughness = metallicRoughnessMap.Sample(textureSampler, input.texCoord).rg;
    float ao = aoMap.Sample(textureSampler, input.texCoord).r;
    
    // 计算最终颜色
    float3 color = albedo.rgb * baseColor.rgb;
    return float4(color, albedo.a);
}
```

### 方法 2：使用 register 指令

```hlsl
// Uniform Buffers
cbuffer MaterialParams : register(b0) {  // Set 0, Binding 0
    float4 baseColor;
    float metallic;
    float roughness;
};

cbuffer LightingParams : register(b1) {  // Set 0, Binding 1
    float4 lightDirection;
    float4 lightColor;
};

// Separate Images
Texture2D albedoMap : register(t2);        // Set 0, Binding 2
Texture2D normalMap : register(t3);        // Set 0, Binding 3
Texture2D metallicRoughnessMap : register(t4);  // Set 0, Binding 4
Texture2D aoMap : register(t5);            // Set 0, Binding 5

// Separate Sampler
SamplerState textureSampler : register(s6);  // Set 0, Binding 6
```

## Binding 分配规则

### register 指令映射

- `register(bN)` - Constant Buffer (Uniform Buffer) → Binding N
- `register(tN)` - Texture/Image → Binding N
- `register(sN)` - Sampler → Binding N
- `register(uN)` - Unordered Access View (Storage Buffer/Image) → Binding N

### Vulkan 属性语法

```hlsl
[[vk::binding(binding_index, set_index)]]
```

- `binding_index` - Binding 编号（从 0 开始）
- `set_index` - Descriptor Set 编号（从 0 开始）

## 正确的 Binding 分配示例

### 示例 1：基本 PBR 材质

```hlsl
// Set 0 的资源分配
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };      // Uniform Buffer
[[vk::binding(1, 0)]] cbuffer LightingParams { ... };      // Uniform Buffer
[[vk::binding(2, 0)]] Texture2D albedoMap;                 // Separate Image
[[vk::binding(3, 0)]] Texture2D normalMap;                 // Separate Image
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;      // Separate Image
[[vk::binding(5, 0)]] Texture2D aoMap;                     // Separate Image
[[vk::binding(6, 0)]] SamplerState textureSampler;         // Separate Sampler
```

### 示例 2：使用 Combined Image Sampler（GLSL 风格）

```hlsl
// 如果使用 Combined Image Sampler（GLSL sampler2D 风格）
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(1, 0)]] cbuffer LightingParams { ... };
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(2, 0)]] SamplerState albedoSampler;  // 注意：可以与 Texture 共享 binding（如果编译器支持）
```

**注意**：在 Vulkan 中，Combined Image Sampler 使用 `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`，而 Separate Image + Sampler 使用 `VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE` 和 `VK_DESCRIPTOR_TYPE_SAMPLER`。

## 常见错误

### ❌ 错误示例 1：Binding 冲突

```hlsl
// 错误：Uniform buffer 和 Texture 使用相同的 binding
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(0, 0)]] Texture2D albedoMap;  // ❌ 冲突！
```

### ❌ 错误示例 2：register 指令冲突

```hlsl
// 错误：Constant buffer 和 Texture 使用相同的 binding
cbuffer MaterialParams : register(b0) { ... };
Texture2D albedoMap : register(t0);  // ❌ 如果 t0 映射到 binding 0，则冲突！
```

### ✅ 正确示例

```hlsl
// 正确：使用不同的 bindings
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(1, 0)]] cbuffer LightingParams { ... };
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] SamplerState textureSampler;
```

## 修复当前问题的建议

根据错误信息，你的着色器当前有：
- `MaterialParams` 在 Binding 0
- `LightingParams` 在 Binding 1
- `albedoMap` 在 Binding 0 ❌（冲突！）
- `normalMap` 在 Binding 1 ❌（冲突！）
- `metallicRoughnessMap` 在 Binding 2
- `aoMap` 在 Binding 3
- `textureSampler` 在 Binding 0 ❌（冲突！）

**修复方案**：

```hlsl
// Uniform Buffers
[[vk::binding(0, 0)]] cbuffer MaterialParams { ... };
[[vk::binding(1, 0)]] cbuffer LightingParams { ... };

// Separate Images - 从 Binding 2 开始
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] Texture2D aoMap;

// Separate Sampler - Binding 6
[[vk::binding(6, 0)]] SamplerState textureSampler;
```

或者使用 register 指令：

```hlsl
cbuffer MaterialParams : register(b0) { ... };
cbuffer LightingParams : register(b1) { ... };
Texture2D albedoMap : register(t2);
Texture2D normalMap : register(t3);
Texture2D metallicRoughnessMap : register(t4);
Texture2D aoMap : register(t5);
SamplerState textureSampler : register(s6);
```

## 注意事项

1. **Binding 编号必须唯一**：在同一个 descriptor set 中，每个 binding 只能有一种资源类型
2. **Set 编号**：可以使用不同的 set 来组织资源（例如，Set 0 用于 per-object，Set 1 用于 per-frame）
3. **Separate vs Combined**：
   - Separate Image + Sampler：需要两个 binding（一个用于 image，一个用于 sampler）
   - Combined Image Sampler：只需要一个 binding（image 和 sampler 组合）
4. **编译器差异**：不同的 HLSL 编译器（glslang vs DXC）对 binding 的处理可能略有不同
