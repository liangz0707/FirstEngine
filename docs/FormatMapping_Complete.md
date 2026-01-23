# 完整的格式映射实现

## 概述

已完整实现 `MapTypeToFormat` 函数，支持所有 SPIRType basetype 和 width 组合，并添加了完整的格式支持。

## 新增格式

### 1. 8-bit 格式
- `R8_UNORM`, `R8_SNORM`, `R8_UINT`, `R8_SINT`
- `R8G8_UNORM`, `R8G8_SNORM`, `R8G8_UINT`, `R8G8_SINT`
- `R8G8B8A8_SNORM` (新增)

### 2. 16-bit 整数格式
- `R16_UINT`, `R16_SINT`, `R16_UNORM`, `R16_SNORM`
- `R16G16_UINT`, `R16G16_SINT`, `R16G16_UNORM`, `R16G16_SNORM`
- `R16G16B16_UINT`, `R16G16B16_SINT`, `R16G16B16_UNORM`, `R16G16B16_SNORM`
- `R16G16B16A16_UINT`, `R16G16B16A16_SINT`, `R16G16B16A16_UNORM`, `R16G16B16A16_SNORM`

### 3. 32-bit 整数格式
- `R32_UINT`, `R32_SINT`
- `R32G32_UINT`, `R32G32_SINT`
- `R32G32B32_UINT`, `R32G32B32_SINT`
- `R32G32B32A32_UINT`, `R32G32B32A32_SINT`

### 4. 64-bit 整数格式
- `R64_UINT`, `R64_SINT`
- `R64G64_UINT`, `R64G64_SINT`
- `R64G64B64_UINT`, `R64G64B64_SINT`
- `R64G64B64A64_UINT`, `R64G64B64A64_SINT`

### 5. 64-bit 浮点格式
- `R64_SFLOAT`, `R64G64_SFLOAT`, `R64G64B64_SFLOAT`, `R64G64B64A64_SFLOAT`

## 完整的 basetype 映射

| basetype | 类型 | 支持的 width | 格式前缀 |
|---------|------|-------------|---------|
| 3 | SByte | 8 | R8_SINT, R8G8_SINT, R8G8B8A8_SINT |
| 4 | UByte | 8 | R8_UINT, R8G8_UINT, R8G8B8A8_UINT |
| 5 | Short | 16 | R16_SINT, R16G16_SINT, R16G16B16_SINT, R16G16B16A16_SINT |
| 6 | UShort | 16 | R16_UINT, R16G16_UINT, R16G16B16_UINT, R16G16B16A16_UINT |
| 7 | Int | 8, 16, 32, 64 | R8/16/32/64_SINT + G/B/A variants |
| 8 | UInt | 8, 16, 32, 64 | R8/16/32/64_UINT + G/B/A variants |
| 9 | Int64 | 64 | R64_SINT, R64G64_SINT, R64G64B64_SINT, R64G64B64A64_SINT |
| 10 | UInt64 | 64 | R64_UINT, R64G64_UINT, R64G64B64_UINT, R64G64B64A64_UINT |
| 12 | Half | 16 | R16_SFLOAT, R16G16_SFLOAT, R16G16B16_SFLOAT, R16G16B16A16_SFLOAT |
| 13 | Float | 16, 32, 64 | R16/32/64_SFLOAT + G/B/A variants |
| 14 | Double | 64 | R64_SFLOAT, R64G64_SFLOAT, R64G64B64_SFLOAT, R64G64B64A64_SFLOAT |

## 映射逻辑

### Float 类型 (basetype == 13)
- `width == 32` → R32*_SFLOAT (4 bytes/component)
- `width == 16` → R16*_SFLOAT (2 bytes/component)
- `width == 64` → R64*_SFLOAT (8 bytes/component)
- `width == 未知` → R16*_SFLOAT (默认，2 bytes/component)

### Half 类型 (basetype == 12)
- 总是 → R16*_SFLOAT (2 bytes/component)

### Double 类型 (basetype == 14)
- 总是 → R64*_SFLOAT (8 bytes/component)

### Int 类型 (basetype == 7)
- `width == 32` → R32*_SINT (4 bytes/component)
- `width == 16` → R16*_SINT (2 bytes/component)
- `width == 8` → R8*_SINT (1 byte/component)
- `width == 64` → R64*_SINT (8 bytes/component)
- `width == 未知` → R32*_SINT (默认，4 bytes/component)

### UInt 类型 (basetype == 8)
- `width == 32` → R32*_UINT (4 bytes/component)
- `width == 16` → R16*_UINT (2 bytes/component)
- `width == 8` → R8*_UINT (1 byte/component)
- `width == 64` → R64*_UINT (8 bytes/component)
- `width == 未知` → R32*_UINT (默认，4 bytes/component)

### Short 类型 (basetype == 5)
- 总是 → R16*_SINT (2 bytes/component)

### UShort 类型 (basetype == 6)
- 总是 → R16*_UINT (2 bytes/component)

### SByte 类型 (basetype == 3)
- 总是 → R8*_SINT (1 byte/component)

### UByte 类型 (basetype == 4)
- 总是 → R8*_UINT (1 byte/component)

### Int64 类型 (basetype == 9)
- 总是 → R64*_SINT (8 bytes/component)

### UInt64 类型 (basetype == 10)
- 总是 → R64*_UINT (8 bytes/component)

## 格式大小计算

`CalculateFormatSize` 函数已更新，支持所有新格式：

| 格式类型 | 字节数/组件 | 总字节数 (vecsize=1/2/3/4) |
|---------|-----------|---------------------------|
| 8-bit | 1 | 1 / 2 / 3→4 / 4 |
| 16-bit | 2 | 2 / 4 / 6 / 8 |
| 32-bit | 4 | 4 / 8 / 12 / 16 |
| 64-bit | 8 | 8 / 16 / 24 / 32 |

## 关键改进

1. **完整的 basetype 支持**: 支持所有常见的 SPIRType basetype (3-14)
2. **完整的 width 支持**: 支持 8, 16, 32, 64 位宽度
3. **正确的格式映射**: Float 使用 SFLOAT，Int 使用 SINT，UInt 使用 UINT
4. **合理的默认值**: 未知类型使用 16-bit float (2 bytes/component)，匹配 Geometry 数据

## 文件修改

1. **include/FirstEngine/RHI/Types.h**: 添加了所有新格式枚举
2. **src/Device/VulkanRHIWrappers.cpp**: 添加了所有格式的 Vulkan 映射
3. **src/Renderer/ShadingMaterial.cpp**: 
   - 完全重写了 `MapTypeToFormat` 函数
   - 更新了 `CalculateFormatSize` 函数

## 验证

现在 `MapTypeToFormat` 函数应该能够：
- ✅ 正确识别 basetype = 13 的 Float 类型
- ✅ 支持所有常见的 basetype (3-14)
- ✅ 根据 width 选择正确的格式
- ✅ 根据 vecsize 选择正确的通道数
- ✅ 提供合理的默认值
