# Shader 类型映射说明

## 概述

Shader 中的类型宽度（width）和 basetype 是从 SPIR-V 反射中提取的，用于将 shader 中的类型映射到 RHI 格式。

## 代码位置

### 1. 类型信息提取

**文件**: `src/Shader/ShaderCompiler.cpp`  
**函数**: `ShaderCompiler::GetReflection()`  
**行数**: 337-376 (stage_inputs), 378-416 (stage_outputs)

```cpp
// 遍历所有 stage inputs (顶点输入)
for (auto& resource : resources.stage_inputs) {
    ShaderResource sr;
    sr.name = resource.name;
    sr.id = resource.id;
    
    // 从 SPIR-V 类型系统中获取类型信息
    auto& type = compiler.get_type(resource.base_type_id);
    
    // 提取类型信息
    sr.basetype = static_cast<uint32_t>(type.basetype);  // 基础类型
    sr.width = type.width;                               // 类型宽度（位）
    sr.vecsize = type.vecsize;                           // 向量大小
    sr.columns = type.columns;                           // 矩阵列数
    
    reflection.stage_inputs.push_back(sr);
}
```

### 2. 类型信息定义

**文件**: `include/FirstEngine/Shader/ShaderCompiler.h`  
**结构**: `ShaderResource`  
**行数**: 33-40

```cpp
struct ShaderResource {
    // Type information (for vertex inputs)
    uint32_t location = 0;    // Location decoration for vertex inputs
    uint32_t component = 0;   // Component decoration
    uint32_t basetype = 0;    // SPIRType basetype
    uint32_t width = 0;       // Type width (e.g., 32 for float32, 16 for float16)
    uint32_t vecsize = 0;     // Vector size (1 = scalar, 2 = vec2, 3 = vec3, 4 = vec4)
    uint32_t columns = 0;     // Matrix columns (0 = not a matrix, 2 = mat2, 3 = mat3, 4 = mat4)
};
```

### 3. 格式映射

**文件**: `src/Renderer/ShadingMaterial.cpp`  
**函数**: `MapTypeToFormat()`  
**行数**: 213-275

```cpp
static RHI::Format MapTypeToFormat(uint32_t basetype, uint32_t width, uint32_t vecsize) {
    if (basetype == 5) { // Float type
        if (width == 32) {
            // 32-bit float -> R32*_SFLOAT formats
        } else if (width == 16) {
            // 16-bit float -> R16*_SFLOAT formats
        }
    }
    // ...
}
```

## SPIRType basetype 枚举值

根据 `spirv_cross` 库的定义（`spirv_common.hpp`），`SPIRType::basetype` 的枚举值如下：

| basetype 值 | SPIRType::Basetype | 说明 | 示例 |
|------------|-------------------|------|------|
| 0 | Unknown | 未知类型 | - |
| 1 | Void | 空类型 | `void` |
| 2 | Boolean | 布尔类型 | `bool` |
| 3 | SByte | 有符号 8 位整数 | `int8_t` |
| 4 | UByte | 无符号 8 位整数 | `uint8_t` |
| 5 | Short | 有符号 16 位整数 | `int16_t` |
| 6 | UShort | 无符号 16 位整数 | `uint16_t` |
| 7 | Int | 有符号 32 位整数 | `int`, `int32_t` |
| 8 | UInt | 无符号 32 位整数 | `uint`, `uint32_t` |
| 9 | Int64 | 有符号 64 位整数 | `int64_t` |
| 10 | UInt64 | 无符号 64 位整数 | `uint64_t` |
| 11 | AtomicCounter | 原子计数器 | - |
| 12 | Half | 16 位浮点数 | `half`, `float16` |
| **13** | **Float** | **32 位浮点数** | **`float`, `float32`** |
| 14 | Double | 64 位浮点数 | `double`, `float64` |
| 15 | Struct | 结构体 | `struct MyStruct { ... }` |
| 16 | Image | 图像类型 | `texture2D`, `image2D` |
| 17 | SampledImage | 采样图像 | `sampler2D` |
| 18 | Sampler | 采样器 | `sampler` |

## 类型宽度 (width)

`width` 表示类型的基础宽度（以位为单位）：

| width 值 | 说明 | 示例 |
|---------|------|------|
| 8 | 8 位 | `int8_t`, `uint8_t` |
| 16 | 16 位 | `int16_t`, `uint16_t`, `half` (16-bit float) |
| 32 | 32 位 | `int32_t`, `uint32_t`, `float` (32-bit float) |
| 64 | 64 位 | `int64_t`, `uint64_t`, `double` (64-bit float) |

## 向量大小 (vecsize)

`vecsize` 表示向量的组件数量：

| vecsize 值 | 说明 | 示例 |
|-----------|------|------|
| 1 | 标量 | `float`, `int` |
| 2 | 2 组件向量 | `vec2`, `float2` |
| 3 | 3 组件向量 | `vec3`, `float3` |
| 4 | 4 组件向量 | `vec4`, `float4` |

## Shader 类型到 basetype/width/vecsize 的映射

### GLSL 示例

```glsl
// Vertex Shader Input
layout(location = 0) in vec3 inPosition;    // basetype=13, width=32, vecsize=3
layout(location = 1) in vec2 inTexCoord;    // basetype=13, width=32, vecsize=2
layout(location = 2) in float inValue;      // basetype=13, width=32, vecsize=1
```

### HLSL 示例

```hlsl
// Vertex Shader Input
struct VertexInput {
    float3 position : POSITION;    // basetype=13, width=32, vecsize=3
    float2 texCoord : TEXCOORD0;   // basetype=13, width=32, vecsize=2
    float4 tangent : TANGENT;      // basetype=13, width=32, vecsize=4
};
```

## 完整映射流程

```
Shader 代码
    ↓
SPIR-V 编译
    ↓
SPIR-V 反射解析 (spirv_cross)
    ↓
提取 SPIRType 信息
    ├─ basetype (基础类型: Float, Int, UInt, etc.)
    ├─ width (类型宽度: 16, 32, 64 bits)
    └─ vecsize (向量大小: 1, 2, 3, 4)
    ↓
ShaderResource 结构
    ↓
MapTypeToFormat() 函数
    ↓
RHI::Format 枚举值
    ↓
Vulkan 格式 (ConvertFormat)
    ↓
GPU 顶点属性格式
```

## 实际映射示例

### 示例 1: `vec3 position`

```glsl
layout(location = 0) in vec3 inPosition;
```

**提取的信息**:
- `basetype = 13` (SPIRType::Float)
- `width = 32` (32-bit)
- `vecsize = 3` (vec3)

**MapTypeToFormat 映射**:
- `RHI::Format::R32G32B32_SFLOAT` (12 bytes)

### 示例 2: `vec2 texCoord`

```glsl
layout(location = 1) in vec2 inTexCoord;
```

**提取的信息**:
- `basetype = 13` (SPIRType::Float)
- `width = 32` (32-bit)
- `vecsize = 2` (vec2)

**MapTypeToFormat 映射**:
- `RHI::Format::R32G32_SFLOAT` (8 bytes)

### 示例 3: `float4 tangent` (HLSL)

```hlsl
float4 tangent : TANGENT;
```

**提取的信息**:
- `basetype = 13` (SPIRType::Float)
- `width = 32` (32-bit)
- `vecsize = 4` (vec4)

**MapTypeToFormat 映射**:
- `RHI::Format::R32G32B32A32_SFLOAT` (16 bytes)

### 示例 4: `half` (16-bit float)

```glsl
layout(location = 0) in half inValue;
```

**提取的信息**:
- `basetype = 12` (SPIRType::Half) 或 `basetype = 13` (SPIRType::Float) with `width = 16`
- `width = 16` (16-bit)
- `vecsize = 1` (scalar)

**MapTypeToFormat 映射**:
- `RHI::Format::R16_SFLOAT` (2 bytes)

## 关键代码位置总结

1. **类型提取**: `src/Shader/ShaderCompiler.cpp:337-376` (stage_inputs)
2. **类型定义**: `include/FirstEngine/Shader/ShaderCompiler.h:33-40` (ShaderResource)
3. **格式映射**: `src/Renderer/ShadingMaterial.cpp:213-275` (MapTypeToFormat)
4. **格式使用**: `src/Renderer/ShadingMaterial.cpp:334` (ParseShaderReflection)

## 相关库

- **spirv_cross**: 用于 SPIR-V 反射和类型信息提取
- **SPIRType**: spirv_cross 中的类型结构，包含 `basetype`, `width`, `vecsize`, `columns` 等字段

## 注意事项

1. **basetype 值**: 直接来自 `spirv_cross::SPIRType::Basetype` 枚举
2. **width 值**: 直接从 `type.width` 获取，表示基础类型的位宽
3. **vecsize 值**: 直接从 `type.vecsize` 获取，表示向量组件数
4. **默认值**: 如果类型提取失败，所有值都设为 0（Unknown）
