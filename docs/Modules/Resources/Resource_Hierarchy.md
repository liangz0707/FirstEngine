# FirstEngine 资源层级结构文档

## 概述

FirstEngine 的资源系统支持层级依赖关系，资源可以引用其他资源，ResourceManager 会自动处理依赖加载和引用计数管理。

## 资源层级结构

```
Model (模型)
├── Mesh (网格) - 多个
│   └── Vertex Data (顶点数据)
│   └── Index Data (索引数据)
│
├── Material (材质) - 多个
│   ├── Texture (贴图) - 多个
│   │   ├── Albedo (反照率贴图)
│   │   ├── Normal (法线贴图)
│   │   ├── MetallicRoughness (金属度粗糙度贴图)
│   │   └── Emissive (自发光贴图)
│   │
│   ├── Shader (着色器) - 字符串引用
│   ├── Parameters (参数)
│   │   ├── Albedo (vec3)
│   │   ├── Metallic (float)
│   │   ├── Roughness (float)
│   │   └── Emissive (vec3)
│   │
│   └── Custom Parameters (自定义参数)
```

## 资源依赖关系

### Model → Material/Mesh
- Model 包含多个 Mesh 和 Material
- 每个 Mesh 可以关联一个 Material
- 加载 Model 时，会自动加载所有依赖的 Material 和 Mesh

### Material → Texture
- Material 可以引用多个 Texture（通过 slot 名称）
- 常见的 Texture slots：
  - `"Albedo"` - 反照率贴图
  - `"Normal"` - 法线贴图
  - `"MetallicRoughness"` - 金属度粗糙度贴图
  - `"Emissive"` - 自发光贴图
- 加载 Material 时，会自动加载所有依赖的 Texture

### Material → Shader
- Material 通过 `shaderName` 字符串引用 Shader
- Shader 路径可以是相对路径或绝对路径

## 路径解析

ResourceManager 支持多种路径解析方式：

1. **绝对路径**：直接使用，如 `C:/models/player.fbx`
2. **相对路径（基于父资源）**：相对于加载资源的路径，如 `textures/albedo.png`
3. **搜索路径**：ResourceManager 维护搜索路径列表，用于解析相对路径

### 示例

```cpp
// 加载模型（位于 models/player.fbx）
auto* model = resourceManager->LoadModel("models/player.fbx");

// Model 内部引用材质 "materials/player.mat"
// ResourceManager 会自动解析为 "models/materials/player.mat"

// Material 内部引用贴图 "textures/albedo.png"
// ResourceManager 会自动解析为 "models/materials/textures/albedo.png"
```

## 使用示例

### 1. 基本资源加载

```cpp
ResourceManager resourceManager;

// 加载模型（会自动加载依赖的 Material 和 Texture）
auto* model = resourceManager->LoadModel("models/player.fbx");

// 获取模型中的材质
auto* material = model->GetMaterial(0);

// 获取材质中的贴图
auto* albedoTexture = material->GetAlbedoTexture();
```

### 2. 手动设置材质贴图路径

```cpp
auto* material = resourceManager->LoadMaterial("materials/player.mat");

// 设置贴图路径（会在 LoadDependencies 时自动加载）
material->SetTexturePath("Albedo", "textures/albedo.png");
material->SetTexturePath("Normal", "textures/normal.png");

// 加载依赖（会自动加载所有贴图）
material->LoadDependencies(&resourceManager, "materials/");
```

### 3. 资源路径解析

```cpp
ResourceManager resourceManager;

// 添加搜索路径
resourceManager.AddSearchPath("assets");
resourceManager.AddSearchPath("resources");

// 加载资源（会自动在搜索路径中查找）
auto* texture = resourceManager->LoadTexture("textures/wood.png", "");
// 会依次尝试：
// - assets/textures/wood.png
// - resources/textures/wood.png
```

## 引用计数管理

资源系统使用引用计数管理资源生命周期：

- **AddRef()**：增加引用计数（当资源被引用时调用）
- **Release()**：减少引用计数（当资源不再被引用时调用）
- **GetRefCount()**：获取当前引用计数

### 自动引用计数

```cpp
// Model 引用 Material
model->GetMaterial(0);  // Material 的引用计数 +1

// Material 引用 Texture
material->SetTexture("Albedo", texture);  // Texture 的引用计数 +1

// 当 Model 被销毁时，会自动 Release 所有依赖的资源
```

## 资源依赖信息

每个资源都维护依赖信息（`ResourceMetadata::dependencies`）：

```cpp
ResourceDependency dep;
dep.type = ResourceDependency::DependencyType::Texture;
dep.path = "textures/albedo.png";
dep.slot = "Albedo";
dep.isRequired = true;

material->GetMetadata().dependencies.push_back(dep);
```

## 实现细节

### ModelResource::Initialize
1. 从 Model 数据创建 MeshResource
2. 通过 ResourceManager 加载 Material（支持相对路径）
3. 建立依赖关系
4. 管理引用计数

### MaterialResource::LoadDependencies
1. 遍历 `m_TexturePaths` 和 `m_Metadata.dependencies`
2. 通过 ResourceManager 解析并加载 Texture
3. 设置 Texture 到对应的 slot
4. 管理引用计数

### ResourceManager::ResolveResourcePath
1. 检查是否为绝对路径
2. 尝试基于 basePath 解析
3. 尝试从搜索路径解析
4. 返回解析后的路径

## 最佳实践

1. **使用相对路径**：资源文件应使用相对路径引用，便于项目迁移
2. **组织资源目录**：按类型组织资源（models/, materials/, textures/）
3. **统一命名**：使用一致的命名规范（如 slot 名称）
4. **及时卸载**：不再使用的资源应及时卸载，释放内存
5. **缓存管理**：ResourceManager 自动缓存已加载的资源，避免重复加载

## 未来扩展

- [ ] 支持资源热重载
- [ ] 支持异步资源加载
- [ ] 支持资源压缩和流式加载
- [ ] 支持资源版本管理
- [ ] 支持资源依赖图可视化
