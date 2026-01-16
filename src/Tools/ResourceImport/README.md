# ResourceImport - 资源导入工具

ResourceImport 是 FirstEngine 的资源导入工具，用于将外部资源文件导入到引擎的资源包中。

## 功能特性

- 支持多种资源类型导入：
  - **Texture**: PNG, JPG, BMP, TGA, DDS, HDR, EXR, KTX 等
  - **Mesh**: OBJ, PLY, STL 等
  - **Model**: FBX, GLTF, GLB, DAE, 3DS, BLEND, X 等
  - **Material**: MAT, JSON 等材质文件

- 自动生成资源 ID 和 XML 元数据文件
- 自动更新资源清单 (resource_manifest.json)
- 支持虚拟路径管理
- 支持文件覆盖选项

## 使用方法

### 基本语法

```bash
ResourceImport import [选项]
```

### 命令

- `import`, `i` - 导入资源文件
- `help`, `h` - 显示帮助信息

### 选项

- `-i, --input <file>` - 输入文件路径（必需）
- `-o, --output <dir>` - 输出目录（默认: build/Package）
- `-t, --type <type>` - 资源类型: texture, mesh, model, material
- `-v, --virtual-path <path>` - 资源的虚拟路径
- `-n, --name <name>` - 资源名称（默认: 文件名不含扩展名）
- `--overwrite` - 覆盖已存在的文件
- `--no-manifest` - 不更新资源清单

### 示例

#### 导入纹理

```bash
# 自动检测类型
ResourceImport import -i texture.png

# 指定类型和名称
ResourceImport import -i texture.png -t texture -n MyTexture

# 指定输出目录和虚拟路径
ResourceImport import -i texture.png -o build/Package -v textures/mytexture
```

#### 导入网格

```bash
ResourceImport import -i mesh.obj -t mesh -n CubeMesh
```

#### 导入模型

```bash
ResourceImport import -i model.fbx -t model -n MyModel
```

#### 导入材质

```bash
ResourceImport import -i material.mat -t material -n DefaultMaterial
```

## 输出结构

导入的资源会被组织到以下目录结构：

```
build/Package/
├── Textures/          # 纹理文件
│   ├── texture.png
│   └── texture.xml
├── Meshes/            # 网格文件
│   ├── mesh.obj
│   └── mesh.xml
├── Models/            # 模型文件
│   ├── model.fbx
│   └── model.xml
├── Materials/          # 材质文件
│   ├── material.mat
│   └── material.xml
└── resource_manifest.json  # 资源清单
```

## 资源 ID 管理

- 每个导入的资源都会自动分配一个全局唯一的 ResourceID
- ResourceID 会记录在 resource_manifest.json 中
- 如果资源已存在（通过路径或虚拟路径匹配），会重用现有的 ResourceID

## 注意事项

1. **Model 导入**: Model 文件导入时会创建基本的 XML 文件，实际的网格和材质会在运行时从模型文件中提取。

2. **Material 导入**: Material 文件导入时会创建基本的 XML 文件，材质参数会在运行时从源文件中解析。

3. **文件覆盖**: 默认情况下，如果输出文件已存在，导入会失败。使用 `--overwrite` 选项可以覆盖已存在的文件。

4. **资源清单**: 默认情况下，导入会自动更新 resource_manifest.json。使用 `--no-manifest` 选项可以跳过此步骤。
