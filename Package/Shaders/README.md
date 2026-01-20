# Shader Demo 说明文档

本目录包含使用 HLSL 语法编写的 Shader 文件，用于 FirstEngine 渲染系统。

## Shader 列表

### 1. GeometryShader
**用途**: 延迟渲染的几何通道（G-Buffer 生成）

**文件**:
- `GeometryShader.vert.hlsl` - 顶点着色器
- `GeometryShader.frag.hlsl` - 片段着色器

**功能**:
- 输出 G-Buffer（Albedo + Metallic, Normal + Roughness, Material）
- 支持法线贴图
- 支持 PBR 材质属性

**输出**:
- Target 0: Albedo (RGB) + Metallic (A)
- Target 1: Normal (RGB) + Roughness (A)
- Target 2: AO (R) + Emission (G)

### 2. PBR
**用途**: 基于物理的渲染（Physically Based Rendering）

**文件**:
- `PBR.vert.hlsl` - 顶点着色器
- `PBR.frag.hlsl` - 片段着色器

**功能**:
- Cook-Torrance BRDF 模型
- 支持法线贴图
- 支持金属度/粗糙度贴图
- 支持环境光遮蔽（AO）

**材质参数**:
- baseColor: 基础颜色
- metallic: 金属度
- roughness: 粗糙度
- ao: 环境光遮蔽

### 3. PostProcess
**用途**: 后处理效果基础框架

**文件**:
- `PostProcess.vert.hlsl` - 全屏四边形顶点着色器
- `PostProcess.frag.hlsl` - 后处理片段着色器

**功能**:
- 全屏后处理基础框架
- 可扩展用于各种后处理效果

**参数**:
- intensity: 处理强度
- time: 时间参数（可用于动画效果）

### 4. Light
**用途**: 延迟渲染的光照通道（点光源、聚光灯）

**文件**:
- `Light.vert.hlsl` - 光源体积顶点着色器
- `Light.frag.hlsl` - 光照计算片段着色器

**功能**:
- 从 G-Buffer 读取数据
- 支持点光源、聚光灯、方向光
- 距离衰减计算
- 聚光灯角度衰减

**光源类型**:
- 0: 点光源（Point Light）
- 1: 聚光灯（Spot Light）
- 2: 方向光（Directional Light）

### 5. IBL
**用途**: 基于图像的光照（Image Based Lighting）

**文件**:
- `IBL.vert.hlsl` - 顶点着色器
- `IBL.frag.hlsl` - IBL 片段着色器

**功能**:
- 使用环境贴图进行光照
- 支持辐照度贴图（Irradiance Map）
- 支持预过滤环境贴图（Prefiltered Environment Map）
- 支持 BRDF 查找表（BRDF LUT）

**纹理要求**:
- irradianceMap: 辐照度立方体贴图
- prefilterMap: 预过滤环境立方体贴图
- brdfLUT: BRDF 查找表（2D 纹理）

### 6. SunLight
**用途**: 方向光（太阳光）渲染，支持阴影

**文件**:
- `SunLight.vert.hlsl` - 顶点着色器
- `SunLight.frag.hlsl` - 片段着色器（带阴影）

**功能**:
- 方向光 PBR 渲染
- 阴影贴图支持
- PCF 阴影过滤
- Cook-Torrance BRDF

**阴影参数**:
- shadowBias: 阴影偏移
- lightSpaceMatrix: 光源空间矩阵

### 7. Tonemapping
**用途**: 色调映射和伽马校正

**文件**:
- `Tonemapping.vert.hlsl` - 全屏四边形顶点着色器
- `Tonemapping.frag.hlsl` - 色调映射片段着色器

**功能**:
- 多种色调映射算法
- 伽马校正
- HDR 到 LDR 转换

**色调映射模式**:
- 0: Reinhard
- 1: ACES Filmic
- 2: Uncharted 2
- 3: 无色调映射

**参数**:
- exposure: 曝光值
- gamma: 伽马值（通常为 2.2）

## 使用说明

### 编译 Shader

这些 HLSL 文件需要通过 Shader 编译器编译为 SPIR-V 格式。编译后的文件应保存为：
- `*.vert.spv` - 顶点着色器 SPIR-V
- `*.frag.spv` - 片段着色器 SPIR-V

### 在 Material 中使用

在 Material XML 文件中指定 Shader 名称（不含扩展名）：

```xml
<Material>
    <Shader>PBR</Shader>
    ...
</Material>
```

Material 文件会自动查找对应的 `.vert.spv` 和 `.frag.spv` 文件。

### 纹理槽位说明

不同 Shader 使用的纹理槽位：

**GeometryShader / PBR / IBL / SunLight**:
- `t0`: Albedo Map
- `t1`: Normal Map
- `t2`: Metallic/Roughness Map
- `t3`: AO Map (GeometryShader) / Irradiance Map (IBL) / Shadow Map (SunLight)
- `t4`: Emission Map (GeometryShader) / Prefilter Map (IBL)
- `t5`: BRDF LUT (IBL)

**PostProcess**:
- `t0`: Input Texture

**Tonemapping**:
- `t0`: HDR Texture

**Light**:
- `t0-t3`: G-Buffer textures (provided by renderer)

## 注意事项

1. **坐标系**: 这些 Shader 使用 Vulkan 坐标系（Y 轴向下，NDC Z 范围 [0, 1]）
2. **纹理采样**: 所有纹理使用 `SamplerState` 进行采样
3. **常量缓冲区**: 使用 `register(b0)`, `register(b1)` 等指定绑定位置
4. **纹理绑定**: 使用 `register(t0)`, `register(t1)` 等指定纹理槽位
5. **采样器绑定**: 使用 `register(s0)`, `register(s1)` 等指定采样器槽位

## 扩展建议

- 可以添加更多后处理效果（Bloom, SSAO, SSR 等）
- 可以添加更多光源类型（区域光等）
- 可以优化 IBL 实现（使用更高质量的预计算）
- 可以添加更多色调映射算法
