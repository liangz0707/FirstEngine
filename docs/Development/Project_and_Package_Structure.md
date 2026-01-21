# 项目（Project）和 Package 的区别

## 概述

在 FirstEngine 编辑器中，**项目（Project）** 和 **Package** 是两个不同的概念，它们有不同的用途和文件结构。

## 项目（Project）

### 定义
**项目** 是编辑器的工作空间概念，用于管理整个游戏/应用开发工作。

### 文件结构
```
项目根目录/
├── project.json              # 项目配置文件（编辑器使用）
│   {
│     "name": "MyGame",
│     "version": "1.0.0"
│   }
└── Package/                  # 资源包目录（见下方说明）
    └── ...
```

### 用途
- 编辑器的工作空间标识
- 存储项目元数据（名称、版本等）
- 作为 Package 目录的父目录
- 可以包含其他项目相关文件（场景文件、配置文件等）

### 特点
- 每个项目有一个 `project.json` 文件
- 项目路径是编辑器打开的工作目录
- 一个项目对应一个 Package 目录

---

## Package（资源包）

### 定义
**Package** 是引擎运行时使用的资源包目录，包含所有可用的资源文件。

### 文件结构
```
Package/
├── resource_manifest.json    # 资源清单（所有资源的索引和映射）
│   {
│     "version": 1,
│     "nextID": 5000,
│     "resources": [
│       {
│         "id": 1001,
│         "path": "Textures/albedo_texture.xml",
│         "virtualPath": "textures/albedo",
│         "type": "Texture"
│       },
│       ...
│     ]
│   }
├── Textures/                 # 纹理资源目录
│   ├── example_texture.xml
│   ├── example_texture.png
│   └── ...
├── Materials/                # 材质资源目录
│   ├── pbr_material.xml
│   └── ...
├── Meshes/                   # 网格资源目录
│   ├── cube_mesh.xml
│   ├── cube.obj
│   └── ...
├── Models/                   # 模型资源目录
│   ├── cube_model.xml
│   └── ...
├── Shaders/                  # 着色器资源目录
│   ├── PBR.vert.hlsl
│   ├── PBR.frag.hlsl
│   └── ...
└── Scenes/                   # 场景资源目录
    ├── example_scene.json
    └── ...
```

### 用途
- **引擎运行时资源库**：引擎启动时会扫描 Package 目录加载所有资源
- **资源管理**：通过 `resource_manifest.json` 管理所有资源的 ID、路径、类型
- **资源导入目标**：编辑器导入的资源都会放入 Package 目录
- **资源组织**：按类型组织资源文件（Textures、Materials、Meshes 等）

### 特点
- 必须包含 `resource_manifest.json` 文件
- 资源文件按类型分类存放在子目录中
- 每个资源都有唯一的 ResourceID
- 资源通过虚拟路径（virtualPath）访问

---

## 关系图

```
┌─────────────────────────────────────┐
│         项目（Project）              │
│  ┌───────────────────────────────┐  │
│  │   project.json                │  │  ← 编辑器配置文件
│  └───────────────────────────────┘  │
│                                      │
│  ┌───────────────────────────────┐  │
│  │      Package/                 │  │  ← 资源包目录
│  │  ┌─────────────────────────┐  │  │
│  │  │ resource_manifest.json  │  │  │  ← 资源清单
│  │  └─────────────────────────┘  │  │
│  │  ├── Textures/                │  │  ← 纹理资源
│  │  ├── Materials/               │  │  ← 材质资源
│  │  ├── Meshes/                  │  │  ← 网格资源
│  │  ├── Models/                  │  │  ← 模型资源
│  │  ├── Shaders/                 │  │  ← 着色器资源
│  │  └── Scenes/                  │  │  ← 场景资源
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

---

## 工作流程

### 1. 创建新项目
```
用户操作：File -> New Project
编辑器操作：
  1. 创建项目目录
  2. 创建 project.json
  3. 创建 Package/ 目录结构
  4. 创建空的 resource_manifest.json
```

### 2. 导入资源
```
用户操作：Tools -> Resource Import 或拖拽文件
编辑器操作：
  1. 复制源文件到 Package/对应类型目录/
  2. 创建资源 XML 描述文件（如需要）
  3. 更新 resource_manifest.json
  4. 刷新资源浏览器显示
```

### 3. 引擎运行时
```
引擎启动时：
  1. 查找 build/Package 或项目路径/Package
  2. 加载 resource_manifest.json
  3. 扫描 Shaders/ 目录加载所有着色器
  4. 按需加载其他资源（通过 ResourceID）
```

---

## 路径示例

### 项目路径
```
G:/MyGame/                    ← 项目根目录
├── project.json
└── Package/
```

### Package 路径（相对项目）
```
Package/                      ← 相对于项目根目录
```

### Package 路径（引擎查找）
```
build/Package/                ← 引擎运行时查找路径（相对于可执行文件）
或
项目路径/Package/             ← 编辑器模式下的路径
```

---

## 关键区别总结

| 特性 | 项目（Project） | Package |
|------|----------------|---------|
| **用途** | 编辑器工作空间 | 引擎资源库 |
| **配置文件** | `project.json` | `resource_manifest.json` |
| **位置** | 用户选择的工作目录 | 项目目录下的 `Package/` 子目录 |
| **管理对象** | 项目元数据、场景配置 | 所有资源文件（纹理、模型、材质等） |
| **使用时机** | 编辑器打开时 | 引擎运行时 |
| **内容** | 项目信息、编辑器配置 | 资源文件、资源清单 |

---

## 常见问题

### Q: 为什么需要两个概念？
**A:** 
- **项目**：编辑器需要知道工作空间在哪里，如何组织文件
- **Package**：引擎需要知道资源在哪里，如何加载资源

### Q: 可以直接使用现有的 Package 目录吗？
**A:** 可以！如果工作空间根目录已经有 Package 目录，编辑器会自动识别。只需要：
1. 在包含 Package 的目录创建 `project.json`
2. 或者直接打开包含 Package 的目录作为项目

### Q: Package 目录可以放在项目外部吗？
**A:** 当前设计是 Package 必须在项目目录下。但引擎运行时可以查找 `build/Package`（相对于可执行文件）。

### Q: 多个项目可以共享一个 Package 吗？
**A:** 当前不支持。每个项目有自己独立的 Package 目录。如果需要共享资源，可以：
1. 复制资源到不同项目的 Package
2. 使用符号链接（高级用法）

---

## 实际使用示例

### 场景 1：新建项目
```
1. 用户：File -> New Project
   选择：G:/MyGame
   
2. 编辑器创建：
   G:/MyGame/
   ├── project.json
   └── Package/
       ├── resource_manifest.json
       ├── Textures/
       ├── Materials/
       └── ...
```

### 场景 2：打开现有 Package
```
1. 用户：File -> Open Project
   选择：G:/AIHUMAN/WorkSpace/FirstEngine（包含 Package 目录）
   
2. 编辑器：
   - 检查是否有 project.json
   - 如果没有，自动创建
   - 加载 Package/resource_manifest.json
   - 显示所有资源
```

### 场景 3：导入资源
```
1. 用户：拖拽 texture.png 到资源浏览器
   
2. 编辑器：
   - 复制到：项目路径/Package/Textures/texture.png
   - 创建：项目路径/Package/Textures/texture_texture.xml
   - 更新：项目路径/Package/resource_manifest.json
   - 刷新资源浏览器显示
```

---

## 总结

- **项目** = 编辑器的工作空间，包含 `project.json`
- **Package** = 引擎的资源库，包含所有资源文件和 `resource_manifest.json`
- **关系**：Package 是项目的一个子目录
- **用途**：项目用于编辑器管理，Package 用于引擎运行时加载资源
