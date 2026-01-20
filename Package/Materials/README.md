# Material 文件说明

本目录包含使用新 Shader 的 Material 示例文件。

## Material 文件列表

### 1. PBR_Material.xml
**Shader**: PBR  
**用途**: 基于物理的渲染材质

**参数**:
- baseColor: 基础颜色 (float4)
- metallic: 金属度 (float)
- roughness: 粗糙度 (float)
- ao: 环境光遮蔽 (float)

**纹理**:
- albedoMap: 反照率贴图
- normalMap: 法线贴图
- metallicRoughnessMap: 金属度/粗糙度贴图
- aoMap: 环境光遮蔽贴图

### 2. Geometry_Material.xml
**Shader**: GeometryShader  
**用途**: 延迟渲染几何通道材质

**参数**:
- baseColor: 基础颜色
- metallic: 金属度
- roughness: 粗糙度
- ao: 环境光遮蔽
- emission: 自发光颜色

**纹理**:
- albedoMap: 反照率贴图
- normalMap: 法线贴图
- metallicRoughnessMap: 金属度/粗糙度贴图
- aoMap: 环境光遮蔽贴图

### 3. IBL_Material.xml
**Shader**: IBL  
**用途**: 基于图像的光照材质

**参数**:
- baseColor: 基础颜色
- metallic: 金属度
- roughness: 粗糙度
- ao: 环境光遮蔽

**纹理**:
- albedoMap: 反照率贴图
- normalMap: 法线贴图
- metallicRoughnessMap: 金属度/粗糙度贴图
- aoMap: 环境光遮蔽贴图

**注意**: IBL Shader 还需要环境贴图（irradianceMap, prefilterMap, brdfLUT），这些通常在渲染器中全局设置。

### 4. SunLight_Material.xml
**Shader**: SunLight  
**用途**: 方向光（太阳光）材质，支持阴影

**参数**:
- baseColor: 基础颜色
- metallic: 金属度
- roughness: 粗糙度
- ao: 环境光遮蔽

**纹理**:
- albedoMap: 反照率贴图
- normalMap: 法线贴图
- metallicRoughnessMap: 金属度/粗糙度贴图
- shadowMap: 阴影贴图

### 5. PostProcess_Material.xml
**Shader**: PostProcess  
**用途**: 后处理效果材质

**参数**:
- intensity: 处理强度

**纹理**:
- inputTexture: 输入渲染纹理

### 6. Tonemapping_Material.xml
**Shader**: Tonemapping  
**用途**: 色调映射材质

**参数**:
- exposure: 曝光值 (默认: 1.0)
- gamma: 伽马值 (默认: 2.2)
- tonemapMode: 色调映射模式 (0: Reinhard, 1: ACES, 2: Uncharted2, 3: None)

**纹理**:
- hdrTexture: HDR 输入纹理

### 7. Light_Material.xml
**Shader**: Light  
**用途**: 光源材质（用于延迟渲染光照通道）

**参数**:
- lightIntensity: 光源强度
- lightRadius: 光源半径（点光源）
- lightType: 光源类型 (0: Point, 1: Spot, 2: Directional)

**注意**: Light Shader 使用 G-Buffer 纹理，这些由渲染器提供，不需要在 Material 中指定。

## 使用说明

### 加载 Material

Material 文件通过 ResourceManager 加载：

```cpp
ResourceManager& resourceManager = ResourceManager::GetInstance();
ResourceHandle handle = resourceManager.Load(resourceID);
MaterialHandle material = handle.material;
```

### 在 ModelComponent 中使用

ModelComponent 会自动加载关联的 Material：

```xml
<Model>
    <Material resourceID="mat_pbr_001"/>
</Model>
```

### 纹理资源 ID

Material 文件中引用的纹理资源 ID（如 `tex_default_albedo`）需要在实际的纹理资源文件中定义。确保：

1. 纹理资源文件存在
2. ResourceID 匹配
3. 纹理文件路径正确

## 自定义 Material

要创建自定义 Material：

1. 复制现有的 Material XML 文件
2. 修改 `<Name>` 和 `<ResourceID>`
3. 修改 `<Shader>` 为所需的 Shader 名称
4. 调整 `<Parameters>` 中的参数值
5. 更新 `<Textures>` 中的纹理资源 ID
6. 更新 `<Dependencies>` 中的依赖关系

## 注意事项

1. **ResourceID 唯一性**: 确保每个 Material 的 ResourceID 是唯一的
2. **纹理依赖**: Material 中引用的纹理资源必须存在
3. **Shader 名称**: Shader 名称必须与 Shader 文件名匹配（不含扩展名）
4. **参数类型**: 确保参数类型与 Shader 中定义的常量缓冲区类型匹配
