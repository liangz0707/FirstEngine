# 资源格式快速参考

## 文件命名约定

- **XML 文件**: `资源名称.xml` 或 `资源名称.tex` / `资源名称.mat` / `资源名称.mesh` / `资源名称.model`
- **二进制文件**: 与 XML 中指定的路径一致

## ResourceID 分配建议

为了避免冲突，建议使用以下 ID 范围：

- **Textures**: 1000-1999
- **Materials**: 2000-2999
- **Meshes**: 3000-3999
- **Models**: 4000-4999

## 常见纹理槽名称

- `Albedo` / `Diffuse`: 基础颜色贴图
- `Normal`: 法线贴图
- `Metallic`: 金属度贴图
- `Roughness`: 粗糙度贴图
- `MetallicRoughness`: 金属度+粗糙度组合贴图
- `AO` / `AmbientOcclusion`: 环境光遮蔽贴图
- `Emissive`: 自发光贴图
- `Lightmap`: 光照贴图（模型级别）

## 常见材质参数

### PBR 材质
- `Albedo` (vec4): 基础颜色
- `Metallic` (float): 金属度 (0.0-1.0)
- `Roughness` (float): 粗糙度 (0.0-1.0)
- `AO` (float): 环境光遮蔽 (0.0-1.0)
- `Emissive` (vec3): 自发光颜色

### 传统材质
- `Diffuse` (vec4): 漫反射颜色
- `Specular` (vec4): 高光颜色
- `Shininess` (float): 高光强度

## 示例代码

### 创建和保存资源

```cpp
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/TextureLoader.h"
#include "FirstEngine/Resources/MaterialLoader.h"

using namespace FirstEngine::Resources;

// 获取 ResourceManager
ResourceManager& rm = ResourceManager::GetInstance();

// 1. 注册并保存纹理
ResourceID texID = rm.GetIDManager().RegisterResource(
    "Textures/my_texture.xml",
    ResourceType::Texture
);

TextureLoader::Save(
    "Textures/my_texture.xml",
    "MyTexture",
    texID,
    "my_texture.png",  // 相对于 XML 的路径
    512, 512, 4, true  // width, height, channels, hasAlpha
);

// 2. 注册并保存材质
ResourceID matID = rm.GetIDManager().RegisterResource(
    "Materials/my_material.xml",
    ResourceType::Material
);

std::unordered_map<std::string, MaterialParameterValue> params;
params["Albedo"] = MaterialParameterValue(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
params["Metallic"] = MaterialParameterValue(0.0f);
params["Roughness"] = MaterialParameterValue(0.5f);

std::unordered_map<std::string, ResourceID> textures;
textures["Albedo"] = texID;

std::vector<ResourceDependency> deps;
ResourceDependency dep;
dep.type = ResourceDependency::DependencyType::Texture;
dep.resourceID = texID;
dep.slot = "Albedo";
dep.isRequired = true;
deps.push_back(dep);

MaterialLoader::Save(
    "Materials/my_material.xml",
    "MyMaterial",
    matID,
    "PBRShader",
    params,
    textures,
    deps
);
```

### 加载资源

```cpp
// 加载资源
ResourceHandle handle = rm.Load(texID);
if (handle.texture) {
    // 使用纹理
    uint32_t width = handle.texture->GetWidth();
    uint32_t height = handle.texture->GetHeight();
    const void* data = handle.texture->GetData();
}

// 加载材质（会自动加载依赖的纹理）
ResourceHandle matHandle = rm.Load(matID);
if (matHandle.material) {
    // 使用材质
    const std::string& shaderName = matHandle.material->GetShaderName();
    TextureHandle albedo = matHandle.material->GetTexture("Albedo");
}
```

## 故障排除

### 资源加载失败

1. **检查 XML 文件路径**: 确保路径正确且文件存在
2. **检查 ResourceID**: 确保 ID 已正确注册
3. **检查依赖资源**: 确保所有依赖的资源 ID 都存在
4. **检查文件路径**: 确保二进制文件（图片、模型）路径相对于 XML 文件正确

### 常见错误

- **"FileNotFound"**: XML 文件或二进制文件不存在
- **"InvalidFormat"**: XML 格式错误或文件类型不支持
- **依赖加载失败**: 依赖的资源 ID 不存在或加载失败

## 文件大小建议

- **纹理**: 
  - 小纹理: 256x256 或 512x512
  - 中纹理: 1024x1024
  - 大纹理: 2048x2048 或 4096x4096
- **网格**: 
  - 简单模型: < 10K 顶点
  - 中等模型: 10K-100K 顶点
  - 复杂模型: > 100K 顶点
