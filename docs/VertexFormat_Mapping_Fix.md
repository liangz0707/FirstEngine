# 顶点格式映射修复

## 问题描述

`MapTypeToFormat` 方法存在以下问题：
1. **精度问题**：Shader 中的 float 类型被错误地映射到 `R8G8B8A8_UNORM`（8 位无符号归一化格式），精度太低
2. **不匹配 Geometry**：格式映射不匹配正常的 Geometry 数据
3. **缺少完整映射**：没有完整实现各种 float 类型的格式映射

## 修复内容

### 1. 添加了完整的格式枚举

在 `include/FirstEngine/RHI/Types.h` 中添加了：

**16-bit float 格式（half precision，2 字节/组件）**：
- `R16_SFLOAT` - 1x 16-bit float = 2 bytes
- `R16G16_SFLOAT` - 2x 16-bit float = 4 bytes
- `R16G16B16_SFLOAT` - 3x 16-bit float = 6 bytes
- `R16G16B16A16_SFLOAT` - 4x 16-bit float = 8 bytes

**32-bit float 格式（full precision，4 字节/组件）**：
- `R32_SFLOAT` - 1x 32-bit float = 4 bytes
- `R32G32_SFLOAT` - 2x 32-bit float = 8 bytes
- `R32G32B32_SFLOAT` - 3x 32-bit float = 12 bytes
- `R32G32B32A32_SFLOAT` - 4x 32-bit float = 16 bytes

### 2. 更新了 Vulkan 格式转换

在 `src/Device/VulkanRHIWrappers.cpp` 的 `ConvertFormat` 函数中添加了所有新格式的 Vulkan 映射。

### 3. 完整实现了格式映射

在 `src/Renderer/ShadingMaterial.cpp` 的 `MapTypeToFormat` 函数中：

**Float 类型映射**：
- `width == 32` (32-bit float) → 使用 `R32*_SFLOAT` 格式（4 字节/组件）
- `width == 16` (16-bit float/half) → 使用 `R16*_SFLOAT` 格式（2 字节/组件）
- `width == 未知` → **默认使用 `R16*_SFLOAT` 格式（2 字节/组件）**，匹配 Geometry 数据

**向量大小映射**：
- `vecsize == 1` (scalar) → 单通道格式（R16_SFLOAT 或 R32_SFLOAT）
- `vecsize == 2` (vec2) → 双通道格式（R16G16_SFLOAT 或 R32G32_SFLOAT）
- `vecsize == 3` (vec3) → 三通道格式（R16G16B16_SFLOAT 或 R32G32B32_SFLOAT）
- `vecsize == 4` (vec4) → 四通道格式（R16G16B16A16_SFLOAT 或 R32G32B32A32_SFLOAT）

### 4. 更新了格式大小计算

在 `CalculateFormatSize` 函数中添加了所有新格式的字节大小计算：
- 16-bit float 格式：2, 4, 6, 8 字节
- 32-bit float 格式：4, 8, 12, 16 字节

## 格式映射表

| Shader 类型 | Width | VecSize | RHI Format | 字节数 |
|------------|-------|---------|------------|--------|
| `float` | 32 | 1 | `R32_SFLOAT` | 4 |
| `vec2` | 32 | 2 | `R32G32_SFLOAT` | 8 |
| `vec3` | 32 | 3 | `R32G32B32_SFLOAT` | 12 |
| `vec4` | 32 | 4 | `R32G32B32A32_SFLOAT` | 16 |
| `half` | 16 | 1 | `R16_SFLOAT` | 2 |
| `vec2<half>` | 16 | 2 | `R16G16_SFLOAT` | 4 |
| `vec3<half>` | 16 | 3 | `R16G16B16_SFLOAT` | 6 |
| `vec4<half>` | 16 | 4 | `R16G16B16A16_SFLOAT` | 8 |
| `float` (未知宽度) | ? | 1 | `R16_SFLOAT` | 2 |
| `vec2` (未知宽度) | ? | 2 | `R16G16_SFLOAT` | 4 |
| `vec3` (未知宽度) | ? | 3 | `R16G16B16_SFLOAT` | 6 |
| `vec4` (未知宽度) | ? | 4 | `R16G16B16A16_SFLOAT` | 8 |

## 关键改进

1. **精度正确**：Float 类型现在使用 SFLOAT 格式，而不是 UNORM
2. **匹配 Geometry**：默认使用 16-bit float（2 字节/组件），匹配典型的 Geometry 数据
3. **完整映射**：支持所有常见的 float 向量类型（scalar, vec2, vec3, vec4）
4. **灵活支持**：同时支持 16-bit 和 32-bit float，根据 shader 中的实际类型选择

## 注意事项

1. **默认行为**：未知宽度的 float 默认使用 16-bit（2 字节/组件），这匹配 Geometry 数据并提供了精度/性能的良好平衡
2. **Int/UInt 类型**：目前使用 16-bit float 作为 fallback，如果将来需要，可以添加 INT/UINT 格式
3. **性能考虑**：16-bit float 提供更好的性能（更小的内存占用和带宽），同时精度对于大多数几何数据已经足够

## 验证

修复后，顶点输入格式现在应该：
- ✅ 正确匹配 shader 中的 float 类型
- ✅ 使用适当的精度（SFLOAT 而不是 UNORM）
- ✅ 匹配 Geometry 数据布局（2 字节/组件用于 float）
- ✅ 支持所有常见的向量类型（scalar, vec2, vec3, vec4）
