# 渲染资源数据流程文档

本文档详细梳理了渲染资源（Mesh、Material、Model等）从文件保存、CPU加载、渲染资源构造到GPU渲染提交的完整数据流程。

## 目录

1. [快速参考](#快速参考)
2. [文件保存格式](#文件保存格式)
3. [CPU资源加载流程](#cpu资源加载流程)
4. [渲染资源构造流程](#渲染资源构造流程)
5. [渲染提交流程](#渲染提交流程)
6. [数据流程图](#数据流程图)
7. [关键数据结构](#关键数据结构)
8. [关键时机和生命周期](#关键时机和生命周期)
9. [数据保存形式总结](#数据保存形式总结)
10. [注意事项](#注意事项)
11. [相关文件](#相关文件)

---

## 快速参考

### 数据流概览

```
文件系统 (XML + 二进制)
    ↓
CPU 资源加载 (ResourceManager → Loader → Resource)
    ├─ MeshResource: 存储顶点/索引数据 (std::vector<uint8_t>)
    ├─ MaterialResource: 存储参数和纹理引用
    └─ ModelResource: 存储 Mesh/Material 引用
    ↓
GPU 资源创建 (RenderResourceManager → RenderResource::DoCreate)
    ├─ RenderGeometry: 创建 GPU 缓冲区 (Staging Buffer → Device Buffer)
    └─ ShadingMaterial: 创建 Pipeline + Descriptor Sets + Uniform Buffers
    ↓
渲染提交 (SceneRenderer → RenderQueue → RenderCommandList)
    ├─ Entity → RenderItem (通过 ModelComponent::CreateRenderItem)
    ├─ RenderItem → RenderCommand (通过 SceneRenderer::SubmitRenderQueue)
    └─ RenderCommand → GPU Command (通过 CommandRecorder)
    ↓
GPU 执行 (Device::SubmitCommandBuffer)
```

### 关键数据转换点

| 阶段 | 输入 | 输出 | 转换方法 |
|-----|------|------|---------|
| **文件加载** | XML + 二进制文件 | CPU 资源对象 | `Loader::Load()` |
| **GPU 创建** | CPU 资源对象 | GPU 资源对象 | `RenderResource::DoCreate()` |
| **参数更新** | CPU 参数数据 | GPU Uniform Buffer | `ShadingMaterial::FlushParametersToGPU()` |
| **渲染提交** | RenderItem | RenderCommand | `SceneRenderer::SubmitRenderQueue()` |
| **命令记录** | RenderCommand | GPU Command | `CommandRecorder::RecordCommands()` |

---

## 文件保存格式

### 1. Mesh 资源

**文件结构**:
- **XML 文件** (`.xml` 或 `.mesh`): 存储元数据和引用
- **几何文件** (`.fbx`, `.obj`, `.dae` 等): 存储实际几何数据（由 Assimp 加载）

**XML 格式** (`ResourceXMLParser::SaveMeshToXML`):
```xml
<Mesh>
    <Name>mesh_name</Name>
    <ResourceID>12345</ResourceID>
    <MeshFile>path/to/geometry.fbx</MeshFile>  <!-- 相对路径或绝对路径 -->
</Mesh>
```

**数据保存形式**:
- **元数据**: 存储在 XML 中（名称、ResourceID、源文件路径）
- **几何数据**: 存储在外部几何文件中（顶点、索引、骨骼等）
- **顶点格式**: 从几何文件自动计算，不存储在 XML 中

**保存位置**: `src/Resources/MeshLoader.cpp:290` → `ResourceXMLParser::SaveMeshToXML`

---

### 2. Material 资源

**文件结构**:
- **XML 文件** (`.xml` 或 `.mat`): 存储所有材质数据

**XML 格式** (`ResourceXMLParser::SaveMaterialToXML`):
```xml
<Material>
    <Name>material_name</Name>
    <ResourceID>12346</ResourceID>
    <Shader>shader_name</Shader>
    <Parameters>
        <Parameter name="albedo" type="vec3">1.0 1.0 1.0</Parameter>
        <Parameter name="metallic" type="float">0.5</Parameter>
        <!-- 更多参数... -->
    </Parameters>
    <Textures>
        <Texture slot="albedo" resourceID="12347"/>
        <Texture slot="normal" resourceID="12348"/>
    </Textures>
    <Dependencies>
        <Dependency type="texture" resourceID="12347" slot="albedo" required="true"/>
    </Dependencies>
</Material>
```

**数据保存形式**:
- **Shader 名称**: 存储在 XML 中，用于查找 ShaderCollection
- **参数**: 存储在 XML 中（float, vec2, vec3, vec4, int, bool, mat3, mat4）
- **纹理引用**: 通过 ResourceID 引用，存储在 Dependencies 中
- **依赖关系**: 存储在 Dependencies 节点中

**保存位置**: `src/Resources/MaterialLoader.cpp:104` → `ResourceXMLParser::SaveMaterialToXML`

---

### 3. Model 资源

**文件结构**:
- **XML 文件** (`.xml`): 存储模型逻辑结构

**XML 格式** (`ResourceXMLParser::SaveModelToXML`):
```xml
<Model>
    <Name>model_name</Name>
    <ResourceID>12349</ResourceID>
    <Meshes>
        <Mesh index="0" resourceID="12345"/>
        <Mesh index="1" resourceID="12346"/>
    </Meshes>
    <Materials>
        <Material index="0" resourceID="12347"/>
        <Material index="1" resourceID="12348"/>
    </Materials>
    <Textures>
        <Texture slot="lightmap" resourceID="12350"/>
    </Textures>
    <Dependencies>
        <Dependency type="mesh" resourceID="12345" slot="0" required="true"/>
        <Dependency type="material" resourceID="12347" slot="0" required="true"/>
    </Dependencies>
</Model>
```

**数据保存形式**:
- **Mesh 引用**: 通过 ResourceID 数组引用，索引对应 Mesh 在模型中的位置
- **Material 引用**: 通过 ResourceID 数组引用，索引对应 Material 在模型中的位置
- **纹理引用**: 通过 ResourceID 引用（如 lightmap）
- **依赖关系**: 存储在 Dependencies 节点中

**保存位置**: `src/Resources/ModelLoader.cpp` → `ResourceXMLParser::SaveModelToXML`

**注意**: Model 是逻辑资源，不包含实际几何数据。几何数据存储在 Mesh 资源中。

---

### 4. Texture 资源

**文件结构**:
- **XML 文件** (`.xml`): 存储元数据和引用
- **图像文件** (`.png`, `.jpg`, `.hdr` 等): 存储实际图像数据

**XML 格式**:
```xml
<Texture>
    <Name>texture_name</Name>
    <ResourceID>12347</ResourceID>
    <ImageFile>path/to/image.png</ImageFile>
    <Width>1024</Width>
    <Height>1024</Height>
    <Channels>4</Channels>
    <HasAlpha>true</HasAlpha>
</Texture>
```

**数据保存形式**:
- **元数据**: 存储在 XML 中（名称、ResourceID、图像文件路径、尺寸、通道数）
- **图像数据**: 存储在外部图像文件中（由 stb_image 加载）

---

## CPU资源加载流程

### 整体流程

```
ResourceManager::Load(ResourceID)
    ↓
ResourceManager::LoadInternal(ResourceID)
    ↓
Resource::Load(ResourceID)  (MeshResource/MaterialResource/ModelResource)
    ↓
Loader::Load(ResourceID)  (MeshLoader/MaterialLoader/ModelLoader)
    ↓
ResourceXMLParser::ParseFromFile()  (解析 XML)
    ↓
加载实际数据 (Assimp/stb_image 等)
    ↓
Resource::LoadDependencies()  (加载依赖资源)
    ↓
Resource 对象初始化完成
```

---

### 1. Mesh 资源加载

**流程** (`src/Resources/MeshResource.cpp:60`):

1. **ResourceManager 解析路径**
   - `ResourceManager::GetResolvedPath(ResourceID)` 获取文件路径
   - 路径解析支持相对路径和搜索路径

2. **MeshLoader 加载**
   - `MeshLoader::Load(ResourceID)` (`src/Resources/MeshLoader.cpp:26`)
   - 解析 XML 文件获取元数据和 MeshFile 路径
   - 使用 Assimp 加载几何文件（`.fbx`, `.obj` 等）
   - 提取顶点数据、索引数据、骨骼数据
   - 构建 `VertexFormat`（灵活顶点格式系统）

3. **MeshResource 初始化**
   - `MeshResource::Load(ResourceID)` (`src/Resources/MeshResource.cpp:60`)
   - 从 `MeshLoader::LoadResult` 获取数据
   - 存储顶点数据 (`m_VertexData: std::vector<uint8_t>`)
   - 存储索引数据 (`m_IndexData: std::vector<uint8_t>`)
   - 存储顶点格式 (`m_VertexFormat: VertexFormat`)
   - 设置元数据 (`m_Metadata`)

4. **依赖加载**
   - Mesh 通常没有依赖（几何数据是自包含的）

**数据来源**:
- **元数据**: XML 文件
- **几何数据**: 外部几何文件（通过 Assimp 加载）
- **顶点格式**: 从几何文件自动计算

**CPU 内存存储**:
- `MeshResource::m_VertexData`: `std::vector<uint8_t>` - 原始顶点数据（灵活格式）
- `MeshResource::m_IndexData`: `std::vector<uint8_t>` - 索引数据（uint32_t 数组）
- `MeshResource::m_VertexFormat`: `VertexFormat` - 顶点格式描述符

---

### 2. Material 资源加载

**流程** (`src/Resources/MaterialResource.cpp:54`):

1. **ResourceManager 解析路径**
   - `ResourceManager::GetResolvedPath(ResourceID)` 获取文件路径

2. **MaterialLoader 加载**
   - `MaterialLoader::Load(ResourceID)` (`src/Resources/MaterialLoader.cpp:19`)
   - 解析 XML 文件获取元数据
   - 提取 Shader 名称
   - 提取参数（MaterialParameterValue）
   - 提取纹理引用（ResourceID）

3. **MaterialResource 初始化**
   - `MaterialResource::Load(ResourceID)` (`src/Resources/MaterialResource.cpp:54`)
   - 从 `MaterialLoader::LoadResult` 获取数据
   - 存储 Shader 名称 (`m_ShaderName`)
   - 存储参数 (`m_Parameters: MaterialParameters`)
   - 存储纹理 ID (`m_TextureIDs: std::unordered_map<std::string, ResourceID>`)
   - 查找并设置 ShaderCollection (`SetShaderCollectionID`)

4. **依赖加载**
   - `MaterialResource::LoadDependencies()` (`src/Resources/MaterialResource.cpp:115`)
   - 加载纹理资源（通过 ResourceManager）
   - 纹理存储在 `m_Textures: std::unordered_map<std::string, TextureHandle>`

**数据来源**:
- **元数据**: XML 文件
- **参数**: XML 文件中的 Parameters 节点
- **纹理**: 通过 ResourceID 从 ResourceManager 加载

**CPU 内存存储**:
- `MaterialResource::m_ShaderName`: `std::string` - Shader 名称
- `MaterialResource::m_Parameters`: `MaterialParameters` - 参数键值对
- `MaterialResource::m_TextureIDs`: `std::unordered_map<std::string, ResourceID>` - 纹理 ID 映射
- `MaterialResource::m_Textures`: `std::unordered_map<std::string, TextureHandle>` - 加载的纹理句柄
- `MaterialResource::m_ParameterData`: `std::vector<uint8_t>` - 序列化的参数数据（用于 GPU 上传）

---

### 3. Model 资源加载

**流程** (`src/Resources/ModelResource.cpp:54`):

1. **ResourceManager 解析路径**
   - `ResourceManager::GetResolvedPath(ResourceID)` 获取文件路径

2. **ModelLoader 加载**
   - `ModelLoader::Load(ResourceID)` (`src/Resources/ModelLoader.cpp:21`)
   - 解析 XML 文件获取元数据
   - 提取 Mesh 引用（ResourceID 数组）
   - 提取 Material 引用（ResourceID 数组）
   - 提取纹理引用（ResourceID）

3. **ModelResource 初始化**
   - `ModelResource::Load(ResourceID)` (`src/Resources/ModelResource.cpp:54`)
   - 从 `ModelLoader::LoadResult` 获取数据
   - 存储 Mesh 引用 (`m_Meshes: std::vector<MeshHandle>`)
   - 存储 Material 引用 (`m_Materials: std::vector<MaterialHandle>`)
   - 存储纹理引用 (`m_Textures: std::unordered_map<std::string, TextureHandle>`)

4. **依赖加载**
   - `ModelResource::LoadDependencies()` (`src/Resources/ModelResource.cpp:110`)
   - 加载 Mesh 资源（通过 ResourceManager）
   - 加载 Material 资源（通过 ResourceManager）
   - 加载纹理资源（通过 ResourceManager）
   - 将加载的资源连接到 ModelResource

**数据来源**:
- **元数据**: XML 文件
- **Mesh/Material**: 通过 ResourceID 从 ResourceManager 加载

**CPU 内存存储**:
- `ModelResource::m_Meshes`: `std::vector<MeshHandle>` - Mesh 句柄数组
- `ModelResource::m_Materials`: `std::vector<MaterialHandle>` - Material 句柄数组
- `ModelResource::m_Textures`: `std::unordered_map<std::string, TextureHandle>` - 纹理句柄映射

---

## 渲染资源构造流程

### 整体流程

```
Resource (CPU) → CreateRenderResource() → RenderResource (GPU)
    ↓
RenderResourceManager::ScheduleCreate()
    ↓
RenderResourceManager::ProcessResources() (在 OnCreateResources 中调用)
    ↓
RenderResource::DoCreate(Device)
    ↓
GPU 资源创建完成
```

---

### 1. Mesh → RenderGeometry

**触发时机**:
- 当需要渲染 Mesh 时，调用 `MeshResource::CreateRenderGeometry()`
- 通常在 `ModelComponent::CreateRenderItem()` 中调用

**流程** (`src/Resources/MeshResource.cpp:121`):

1. **创建 RenderGeometry**
   - `MeshResource::CreateRenderGeometry()` (`src/Resources/MeshResource.cpp:121`)
   - 创建 `RenderGeometry` 对象
   - 调用 `RenderGeometry::InitializeFromMesh(MeshResource*)`
   - 设置几何数据（顶点数、索引数、步长等）

2. **调度创建**
   - `RenderGeometry::ScheduleCreate()` (`include/FirstEngine/Renderer/IRenderResource.h`)
   - 将 RenderGeometry 添加到 `RenderResourceManager` 的创建队列

3. **GPU 资源创建**
   - `RenderResourceManager::ProcessResources()` (`src/Renderer/RenderResourceManager.cpp`)
   - 调用 `RenderGeometry::DoCreate(Device)` (`src/Renderer/RenderGeometry.cpp:39`)
   - 创建 device-local 顶点缓冲区
   - 创建 device-local 索引缓冲区
   - 使用 Staging Buffer 上传数据到 GPU

**数据转换**:
- **CPU**: `MeshResource::m_VertexData` (原始字节) → **GPU**: `RenderGeometry::m_VertexBuffer` (RHI::IBuffer*)
- **CPU**: `MeshResource::m_IndexData` (原始字节) → **GPU**: `RenderGeometry::m_IndexBuffer` (RHI::IBuffer*)

**GPU 资源存储**:
- `RenderGeometry::m_VertexBuffer`: `std::unique_ptr<RHI::IBuffer>` - GPU 顶点缓冲区（device-local）
- `RenderGeometry::m_IndexBuffer`: `std::unique_ptr<RHI::IBuffer>` - GPU 索引缓冲区（device-local）

---

### 2. Material → ShadingMaterial

**触发时机**:
- 当需要渲染 Material 时，调用 `MaterialResource::CreateShadingMaterial()`
- 通常在 `ModelComponent::CreateRenderItem()` 中调用

**流程** (`src/Resources/MaterialResource.cpp:304`):

1. **创建 ShadingMaterial**
   - `MaterialResource::CreateShadingMaterial()` (`src/Resources/MaterialResource.cpp:304`)
   - 创建 `ShadingMaterial` 对象
   - 调用 `ShadingMaterial::InitializeFromMaterial(MaterialResource*)` (`src/Renderer/ShadingMaterial.cpp:36`)
   - 从 MaterialResource 获取 ShaderCollection
   - 解析 Shader 反射信息
   - 初始化 Uniform Buffers（从 MaterialResource 参数）
   - 初始化纹理绑定

2. **调度创建**
   - `ShadingMaterial::ScheduleCreate()` (`include/FirstEngine/Renderer/IRenderResource.h`)
   - 将 ShadingMaterial 添加到 `RenderResourceManager` 的创建队列

3. **GPU 资源创建**
   - `RenderResourceManager::ProcessResources()` (`src/Renderer/RenderResourceManager.cpp`)
   - 调用 `ShadingMaterial::DoCreate(Device)` (`src/Renderer/ShadingMaterial.cpp`)
   - 创建 Graphics Pipeline（从 ShaderCollection）
   - 创建 Descriptor Set Layouts（从 Shader 反射）
   - 创建 Descriptor Pool
   - 分配 Descriptor Sets
   - 创建 Uniform Buffers（GPU 内存）
   - 上传参数数据到 Uniform Buffers
   - 绑定纹理到 Descriptor Sets

**数据转换**:
- **CPU**: `MaterialResource::m_Parameters` (键值对) → **GPU**: `ShadingMaterial::m_UniformBuffers` (RHI::IBuffer*)
- **CPU**: `MaterialResource::m_Textures` (TextureHandle) → **GPU**: `ShadingMaterial::m_TextureBindings` (Descriptor Sets)
- **CPU**: `MaterialResource::m_ShaderName` (字符串) → **GPU**: `ShadingMaterial::m_ShadingState::m_Pipeline` (RHI::IPipeline*)

**GPU 资源存储**:
- `ShadingMaterial::m_ShadingState::m_Pipeline`: `RHI::IPipeline*` - Graphics Pipeline
- `ShadingMaterial::m_DescriptorManager`: `MaterialDescriptorManager` - Descriptor Set 管理
- `ShadingMaterial::m_UniformBuffers`: `std::vector<UniformBufferBinding>` - Uniform Buffer 绑定
- `ShadingMaterial::m_TextureBindings`: `std::vector<TextureBinding>` - 纹理绑定

---

## 渲染提交流程

### 整体流程

```
Scene → SceneRenderer::Render()
    ↓
SceneRenderer::BuildRenderQueue() (视锥剔除)
    ↓
SceneRenderer::EntityToRenderItems() (Entity → RenderItem)
    ↓
RenderQueue (RenderItem 列表)
    ↓
RenderQueue::RebuildBatches() (按 Pipeline/Material 分组)
    ↓
SceneRenderer::SubmitRenderQueue() (RenderItem → RenderCommand)
    ↓
RenderCommandList (渲染命令数据)
    ↓
FrameGraph::Execute() (合并到 Pass)
    ↓
CommandRecorder::RecordCommands() (RenderCommand → GPU Command)
    ↓
ICommandBuffer (GPU 命令缓冲区)
    ↓
Device::SubmitCommandBuffer() (提交到 GPU)
```

---

### 1. Scene → RenderQueue

**流程** (`src/Renderer/SceneRenderer.cpp:52`):

1. **视锥剔除**
   - `SceneRenderer::BuildRenderQueue()` (`src/Renderer/SceneRenderer.cpp:52`)
   - 从 Scene 获取所有 Entity
   - 使用视锥剔除（如果启用）
   - 获取可见 Entity 列表

2. **Entity → RenderItem**
   - `SceneRenderer::EntityToRenderItems()` (`src/Renderer/SceneRenderer.cpp:144`)
   - 遍历 Entity 的 Components
   - 对每个 Component（如 ModelComponent）:
     - **获取资源**: 从 ModelResource 获取 MeshResource 和 MaterialResource
     - **确保 GPU 资源已创建**: 
       - 调用 `MeshResource::CreateRenderGeometry()` 确保 RenderGeometry 已创建
       - 调用 `MaterialResource::CreateShadingMaterial()` 确保 ShadingMaterial 已创建
     - **收集渲染参数** (每帧更新):
       - `RenderParameterCollector::CollectFromComponent()` - 从 Component 收集
       - `RenderParameterCollector::CollectFromMaterialResource()` - 从 MaterialResource 收集
       - `RenderParameterCollector::CollectFromCamera()` - 从 Camera 收集（可选）
     - **应用参数**:
       - `ShadingMaterial::ApplyParameters(collector)` - 应用到 CPU 端数据
       - `ShadingMaterial::UpdateRenderParameters()` - 更新 CPU 端 Uniform Buffer 数据
       - `ShadingMaterial::FlushParametersToGPU(device)` - 上传到 GPU Uniform Buffers
     - **创建 RenderItem**:
       - 调用 `ModelComponent::CreateRenderItem()` (`src/Resources/ModelComponent.cpp:195`)
       - 从 MeshResource 获取 RenderData (包含 vertexBuffer, indexBuffer)
       - 从 MaterialResource 获取 RenderData (包含 shadingMaterial, pipeline, descriptorSet)
       - 组合成 RenderItem 并返回

3. **RenderItem 结构**
   ```cpp
   struct RenderItem {
       GeometryData {
           void* vertexBuffer;      // RenderGeometry::m_VertexBuffer
           void* indexBuffer;        // RenderGeometry::m_IndexBuffer
           uint32_t vertexCount;
           uint32_t indexCount;
           // ...
       } geometryData;
       
       MaterialData {
           void* shadingMaterial;   // ShadingMaterial*
           void* pipeline;          // ShadingMaterial::m_ShadingState::m_Pipeline
           void* descriptorSet;      // ShadingMaterial::m_DescriptorManager::GetDescriptorSet()
           // ...
       } materialData;
       
       glm::mat4 worldMatrix;       // Entity::GetWorldMatrix()
       // ...
   };
   ```

4. **添加到 RenderQueue**
   - `RenderQueue::AddItem(RenderItem)` (`src/Renderer/RenderBatch.cpp:83`)
   - RenderQueue 自动按 Pipeline/Material 分组为 Batches

**数据来源**:
- **几何数据**: `MeshResource::GetRenderData()` → `RenderGeometry::GetVertexBuffer/GetIndexBuffer`
- **材质数据**: `MaterialResource::GetRenderData()` → `ShadingMaterial::GetShadingState()`
- **变换矩阵**: `Entity::GetWorldMatrix()` (缓存的，自动更新)

---

### 2. RenderQueue → RenderCommandList

**流程** (`src/Renderer/SceneRenderer.cpp:222`):

1. **分组为 Batches**
   - `RenderQueue::RebuildBatches()` (`src/Renderer/RenderBatch.cpp:112`)
   - 按 ShadingMaterial（或 Pipeline + DescriptorSet）分组
   - 每个 Batch 包含使用相同 Pipeline/Material 的 RenderItems

2. **转换为 RenderCommand**
   - `SceneRenderer::SubmitRenderQueue()` (`src/Renderer/SceneRenderer.cpp:222`)
   - 遍历每个 Batch
   - 对每个 RenderItem:
     - **BindPipeline**: 如果 Pipeline 改变
     - **BindDescriptorSets**: 如果 DescriptorSet 改变
     - **BindVertexBuffers**: 绑定顶点缓冲区
     - **BindIndexBuffer**: 绑定索引缓冲区
     - **DrawIndexed** 或 **Draw**: 绘制命令

3. **RenderCommand 结构**
   ```cpp
   enum class RenderCommandType {
       BindPipeline,
       BindDescriptorSets,
       BindVertexBuffers,
       BindIndexBuffer,
       Draw,
       DrawIndexed,
       BeginRenderPass,
       EndRenderPass,
       // ...
   };
   
   struct RenderCommand {
       RenderCommandType type;
       union {
           BindPipelineParams bindPipeline;
           BindDescriptorSetsParams bindDescriptorSets;
           BindVertexBuffersParams bindVertexBuffers;
           BindIndexBufferParams bindIndexBuffer;
           DrawParams draw;
           DrawIndexedParams drawIndexed;
           // ...
       } params;
   };
   ```

**数据转换**:
- **RenderItem** (数据结构) → **RenderCommand** (命令数据)
- 每个 RenderItem 生成多个 RenderCommand（BindPipeline, BindDescriptorSets, BindVertexBuffers, BindIndexBuffer, DrawIndexed）

**RenderItem 创建细节** (`src/Resources/ModelComponent.cpp:186`):
- `ModelComponent::CreateRenderItem()` 从 ModelResource 获取 Mesh 和 Material
- 调用 `MeshResource::GetRenderData()` 获取几何数据（vertexBuffer, indexBuffer）
- 调用 `MaterialResource::GetRenderData()` 获取材质数据（shadingMaterial, pipeline, descriptorSet）
- 验证顶点输入匹配（`ShadingMaterial::ValidateVertexInputs()`）
- 计算排序键（基于 Pipeline、Material、深度）

---

### 3. RenderCommandList → GPU CommandBuffer

**流程** (`src/Renderer/CommandRecorder.cpp:13`):

1. **FrameGraph 执行**
   - `FrameGraph::Execute()` (`src/Renderer/FrameGraph.cpp:430`)
   - 按执行计划顺序执行每个 Pass
   - 对于 GeometryPass:
     - 获取 SceneRenderer 的 RenderCommandList
     - 合并到 Pass 的 RenderCommandList
   - 对于 LightingPass/PostProcessPass:
     - 生成 Pass 特定的 RenderCommandList

2. **记录到 CommandBuffer**
   - `CommandRecorder::RecordCommands()` (`src/Renderer/CommandRecorder.cpp:13`)
   - 遍历 RenderCommandList
   - 对每个 RenderCommand:
     - 调用对应的 `RecordXXX()` 方法
     - 转换为 RHI 命令并记录到 CommandBuffer

3. **RHI 命令记录**
   - `VulkanCommandBuffer::BindPipeline()` → `vkCmdBindPipeline()`
   - `VulkanCommandBuffer::BindDescriptorSets()` → `vkCmdBindDescriptorSets()`
   - `VulkanCommandBuffer::BindVertexBuffers()` → `vkCmdBindVertexBuffers()`
   - `VulkanCommandBuffer::BindIndexBuffer()` → `vkCmdBindIndexBuffer()`
   - `VulkanCommandBuffer::DrawIndexed()` → `vkCmdDrawIndexed()`

4. **提交到 GPU**
   - `Device::SubmitCommandBuffer()` (`src/Device/VulkanDevice.cpp`)
   - 提交 CommandBuffer 到 Graphics Queue
   - GPU 执行渲染命令

**数据转换**:
- **RenderCommand** (命令数据) → **Vulkan Command** (GPU 命令)
- 通过 `VulkanCommandBuffer` 记录到 `VkCommandBuffer`

---

## 数据流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                        文件系统                                  │
├─────────────────────────────────────────────────────────────────┤
│  Mesh:    mesh.xml + geometry.fbx                               │
│  Material: material.xml                                          │
│  Model:   model.xml                                              │
│  Texture: texture.xml + image.png                                │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────────────┐
│                    CPU 资源加载阶段                              │
├─────────────────────────────────────────────────────────────────┤
│  ResourceManager::Load(ResourceID)                               │
│    ↓                                                             │
│  Loader::Load(ResourceID)                                        │
│    ├─ ResourceXMLParser::ParseFromFile()  (解析 XML)            │
│    ├─ Assimp::ReadFile()  (加载几何文件)                        │
│    └─ stb_image::load()  (加载图像文件)                        │
│    ↓                                                             │
│  Resource::Load(ResourceID)                                     │
│    ├─ MeshResource::Load()                                      │
│    │   └─ 存储: m_VertexData, m_IndexData, m_VertexFormat      │
│    ├─ MaterialResource::Load()                                   │
│    │   └─ 存储: m_ShaderName, m_Parameters, m_Textures         │
│    └─ ModelResource::Load()                                     │
│        └─ 存储: m_Meshes, m_Materials, m_Textures               │
│    ↓                                                             │
│  Resource::LoadDependencies()  (加载依赖资源)                   │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────────────┐
│                 渲染资源构造阶段                                  │
├─────────────────────────────────────────────────────────────────┤
│  Resource::CreateRenderResource()                               │
│    ↓                                                             │
│  RenderResourceManager::ScheduleCreate()  (调度创建)             │
│    ↓                                                             │
│  RenderResourceManager::ProcessResources()  (处理队列)           │
│    ↓                                                             │
│  RenderResource::DoCreate(Device)                               │
│    ├─ RenderGeometry::DoCreate()                                │
│    │   ├─ 创建 Staging Buffer (HostVisible)                     │
│    │   ├─ 上传数据到 Staging Buffer                             │
│    │   ├─ 创建 Device Buffer (DeviceLocal)                      │
│    │   └─ CopyBuffer (Staging → Device)                         │
│    │                                                             │
│    └─ ShadingMaterial::DoCreate()                               │
│        ├─ 创建 Graphics Pipeline                                │
│        ├─ 创建 Descriptor Set Layouts                           │
│        ├─ 创建 Descriptor Pool                                   │
│        ├─ 分配 Descriptor Sets                                  │
│        ├─ 创建 Uniform Buffers                                  │
│        ├─ 上传参数到 Uniform Buffers                            │
│        └─ 绑定纹理到 Descriptor Sets                            │
└───────────────────────┬─────────────────────────────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────────────┐
│                   渲染提交阶段                                    │
├─────────────────────────────────────────────────────────────────┤
│  SceneRenderer::Render()                                        │
│    ↓                                                             │
│  BuildRenderQueue()  (视锥剔除)                                 │
│    ↓                                                             │
│  EntityToRenderItems()  (Entity → RenderItem)                    │
│    ├─ 获取 RenderGeometry (GPU Buffer)                          │
│    ├─ 获取 ShadingMaterial (GPU Pipeline/DescriptorSet)        │
│    ├─ 收集渲染参数 (每帧更新)                                    │
│    ├─ ApplyParameters()  (应用到 ShadingMaterial)               │
│    ├─ UpdateRenderParameters()  (更新 CPU 端 Uniform Buffer)    │
│    └─ FlushParametersToGPU()  (上传到 GPU)                      │
│    ↓                                                             │
│  RenderQueue (RenderItem 列表)                                  │
│    ↓                                                             │
│  RebuildBatches()  (按 Pipeline/Material 分组)                 │
│    ↓                                                             │
│  SubmitRenderQueue()  (RenderItem → RenderCommand)              │
│    ├─ BindPipeline                                              │
│    ├─ BindDescriptorSets                                        │
│    ├─ BindVertexBuffers                                         │
│    ├─ BindIndexBuffer                                           │
│    └─ DrawIndexed                                               │
│    ↓                                                             │
│  RenderCommandList (命令数据)                                   │
│    ↓                                                             │
│  FrameGraph::Execute()  (合并到 Pass)                           │
│    ↓                                                             │
│  CommandRecorder::RecordCommands()  (命令数据 → GPU 命令)       │
│    ↓                                                             │
│  ICommandBuffer (GPU 命令缓冲区)                                │
│    ↓                                                             │
│  Device::SubmitCommandBuffer()  (提交到 GPU)                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## 关键数据结构

### CPU 端数据结构

#### MeshResource
```cpp
class MeshResource {
    // CPU 数据
    std::vector<uint8_t> m_VertexData;      // 原始顶点数据（灵活格式）
    std::vector<uint8_t> m_IndexData;       // 索引数据
    VertexFormat m_VertexFormat;             // 顶点格式描述符
    uint32_t m_VertexCount;
    uint32_t m_IndexCount;
    uint32_t m_VertexStride;
    
    // GPU 资源（延迟创建）
    void* m_RenderGeometry;                  // RenderGeometry*
};
```

#### MaterialResource
```cpp
class MaterialResource {
    // CPU 数据
    std::string m_ShaderName;                // Shader 名称
    MaterialParameters m_Parameters;         // 参数键值对
    std::vector<uint8_t> m_ParameterData;   // 序列化的参数数据
    std::unordered_map<std::string, TextureHandle> m_Textures;  // 纹理句柄
    std::unordered_map<std::string, ResourceID> m_TextureIDs;    // 纹理 ID
    
    // GPU 资源（延迟创建）
    void* m_ShadingMaterial;                 // ShadingMaterial*
};
```

#### ModelResource
```cpp
class ModelResource {
    // CPU 数据（引用）
    std::vector<MeshHandle> m_Meshes;       // Mesh 句柄数组
    std::vector<MaterialHandle> m_Materials; // Material 句柄数组
    std::unordered_map<std::string, TextureHandle> m_Textures;  // 纹理句柄
};
```

---

### GPU 端数据结构

#### RenderGeometry
```cpp
class RenderGeometry {
    // GPU 资源
    std::unique_ptr<RHI::IBuffer> m_VertexBuffer;  // Device-local 顶点缓冲区
    std::unique_ptr<RHI::IBuffer> m_IndexBuffer;   // Device-local 索引缓冲区
    
    // 几何数据（从 MeshResource 复制）
    uint32_t m_VertexCount;
    uint32_t m_IndexCount;
    uint32_t m_VertexStride;
};
```

#### ShadingMaterial
```cpp
class ShadingMaterial {
    // GPU 资源
    ShadingState m_ShadingState;            // 包含 Pipeline
    MaterialDescriptorManager m_DescriptorManager;  // Descriptor Set 管理
    std::vector<UniformBufferBinding> m_UniformBuffers;  // Uniform Buffer 绑定
    std::vector<TextureBinding> m_TextureBindings;      // 纹理绑定
    
    // CPU 数据（用于每帧更新）
    std::vector<uint8_t> m_UniformBufferData;  // CPU 端 Uniform Buffer 数据
};
```

---

### 渲染数据结构

#### RenderItem
```cpp
struct RenderItem {
    GeometryData {
        void* vertexBuffer;                  // RenderGeometry::m_VertexBuffer
        void* indexBuffer;                   // RenderGeometry::m_IndexBuffer
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t firstIndex;
        uint32_t firstVertex;
    } geometryData;
    
    MaterialData {
        void* shadingMaterial;              // ShadingMaterial*
        void* pipeline;                      // RHI::IPipeline*
        void* descriptorSet;                 // Descriptor Set Handle
    } materialData;
    
    glm::mat4 worldMatrix;                   // 世界变换矩阵
    uint64_t sortKey;                        // 排序键
};
```

#### RenderCommand
```cpp
enum class RenderCommandType {
    BindPipeline,
    BindDescriptorSets,
    BindVertexBuffers,
    BindIndexBuffer,
    Draw,
    DrawIndexed,
    BeginRenderPass,
    EndRenderPass,
    // ...
};

struct RenderCommand {
    RenderCommandType type;
    union {
        BindPipelineParams bindPipeline;
        BindDescriptorSetsParams bindDescriptorSets;
        BindVertexBuffersParams bindVertexBuffers;
        BindIndexBufferParams bindIndexBuffer;
        DrawParams draw;
        DrawIndexedParams drawIndexed;
        // ...
    } params;
};
```

---

## 数据流总结

### 1. 文件 → CPU 资源

| 资源类型 | XML 文件 | 二进制文件 | CPU 存储 |
|---------|---------|-----------|---------|
| **Mesh** | 元数据 + MeshFile 路径 | 几何文件 (.fbx, .obj) | `m_VertexData`, `m_IndexData`, `m_VertexFormat` |
| **Material** | 所有数据（参数、纹理引用） | 无 | `m_ShaderName`, `m_Parameters`, `m_Textures` |
| **Model** | 逻辑结构（Mesh/Material 引用） | 无 | `m_Meshes`, `m_Materials`, `m_Textures` |
| **Texture** | 元数据 + ImageFile 路径 | 图像文件 (.png, .jpg) | `m_ImageData` |

### 2. CPU 资源 → GPU 资源

| CPU 资源 | GPU 资源 | 转换方式 |
|---------|---------|---------|
| **MeshResource** | **RenderGeometry** | Staging Buffer 上传 |
| **MaterialResource** | **ShadingMaterial** | Uniform Buffer + Descriptor Set |
| **TextureResource** | **RHI::IImage** | Image Upload |

### 3. GPU 资源 → 渲染命令

| GPU 资源 | 渲染命令 | 数据来源 |
|---------|---------|---------|
| **RenderGeometry** | BindVertexBuffers, BindIndexBuffer, DrawIndexed | `RenderItem::geometryData` |
| **ShadingMaterial** | BindPipeline, BindDescriptorSets | `RenderItem::materialData` |
| **Entity** | WorldMatrix (PushConstants) | `Entity::GetWorldMatrix()` |

### 4. 渲染命令 → GPU 执行

| 渲染命令 | GPU 命令 | 执行时机 |
|---------|---------|---------|
| **RenderCommandList** | **VkCommandBuffer** | `CommandRecorder::RecordCommands()` |
| **VkCommandBuffer** | **GPU Queue** | `Device::SubmitCommandBuffer()` |

---

## 关键时机和生命周期

### 资源创建时机

1. **CPU 资源加载**: 
   - 时机: `ResourceManager::Load(ResourceID)` 调用时
   - 位置: 应用启动、场景加载、按需加载
   - 触发: Entity 加载时自动加载依赖资源

2. **GPU 资源创建**:
   - 时机: `RenderResourceManager::ProcessResources()` 调用时
   - 位置: `RenderApp::OnCreateResources()` 或 `RenderContext::ProcessResources()`
   - 方式: 延迟创建（Lazy Creation），只在需要时创建
   - 触发: 
     - `ModelComponent::OnLoad()` 时调用 `CreateRenderGeometry()` 和 `CreateShadingMaterial()`
     - 或首次调用 `GetRenderData()` 时检查并创建

3. **参数更新**:
   - 时机: 每帧渲染前
   - 位置: `SceneRenderer::EntityToRenderItems()` → `ShadingMaterial::FlushParametersToGPU()`
   - 方式: CPU 端更新 Uniform Buffer 数据，然后上传到 GPU
   - 流程:
     1. `RenderParameterCollector` 收集参数（从 Component、MaterialResource、Camera）
     2. `ShadingMaterial::ApplyParameters()` - 应用到 CPU 端数据
     3. `ShadingMaterial::UpdateRenderParameters()` - 更新 CPU 端 Uniform Buffer 数据
     4. `ShadingMaterial::FlushParametersToGPU()` - 上传到 GPU Uniform Buffers

### 资源生命周期

1. **CPU 资源**:
   - 创建: `ResourceManager::Load()`
   - 缓存: `ResourceManager` 单例缓存
   - 销毁: 引用计数为 0 时自动销毁

2. **GPU 资源**:
   - 创建: `RenderResource::DoCreate()`
   - 缓存: `RenderResourceManager` 管理
   - 销毁: `RenderResource::DoDestroy()` (在 RenderResourceManager 清理时)

3. **渲染命令**:
   - 创建: 每帧在 `FrameGraph::Execute()` 中生成
   - 生命周期: 单帧，执行后丢弃

---

## 数据保存形式总结

### 文件格式

1. **XML 文件**: 存储元数据、引用、参数
   - 格式: pugixml
   - 位置: `ResourceXMLParser`

2. **二进制文件**: 存储实际数据
   - Mesh: 几何文件（Assimp 格式）
   - Texture: 图像文件（stb_image 格式）

### 内存格式

1. **CPU 内存**: 
   - 原始字节数组 (`std::vector<uint8_t>`)
   - 结构化数据（参数键值对、句柄数组）

2. **GPU 内存**:
   - Device-local 缓冲区（顶点、索引、Uniform）
   - Descriptor Sets（纹理、Uniform Buffer 绑定）

---

## 数据流详细表格

### Mesh 资源数据流

| 阶段 | 数据形式 | 存储位置 | 数据结构 |
|-----|---------|---------|---------|
| **文件** | XML + .fbx/.obj | 文件系统 | XML: 元数据<br>几何文件: 二进制几何数据 |
| **CPU 加载** | 原始字节 | `MeshResource::m_VertexData` | `std::vector<uint8_t>` |
| **GPU 创建** | GPU 缓冲区 | `RenderGeometry::m_VertexBuffer` | `RHI::IBuffer*` (device-local) |
| **渲染提交** | 缓冲区指针 | `RenderItem::geometryData.vertexBuffer` | `void*` (RHI::IBuffer*) |
| **GPU 命令** | Vulkan 命令 | `VkCommandBuffer` | `vkCmdBindVertexBuffers()` |

### Material 资源数据流

| 阶段 | 数据形式 | 存储位置 | 数据结构 |
|-----|---------|---------|---------|
| **文件** | XML | 文件系统 | XML: 参数、纹理引用 |
| **CPU 加载** | 键值对 | `MaterialResource::m_Parameters` | `MaterialParameters` (unordered_map) |
| **GPU 创建** | Uniform Buffer | `ShadingMaterial::m_UniformBuffers` | `RHI::IBuffer*` (device-local) |
| **参数更新** | 每帧更新 | CPU → GPU | `FlushParametersToGPU()` |
| **渲染提交** | Descriptor Set | `RenderItem::materialData.descriptorSet` | `void*` (Descriptor Set Handle) |
| **GPU 命令** | Vulkan 命令 | `VkCommandBuffer` | `vkCmdBindDescriptorSets()` |

### Model 资源数据流

| 阶段 | 数据形式 | 存储位置 | 数据结构 |
|-----|---------|---------|---------|
| **文件** | XML | 文件系统 | XML: Mesh/Material 引用 |
| **CPU 加载** | 句柄数组 | `ModelResource::m_Meshes`<br>`ModelResource::m_Materials` | `std::vector<MeshHandle>`<br>`std::vector<MaterialHandle>` |
| **渲染提交** | RenderItem | `RenderQueue` | `RenderItem` (组合 Mesh + Material) |

---

## 注意事项

1. **资源引用**: 所有资源通过 ResourceID 引用，不直接使用文件路径
2. **依赖加载**: 依赖资源通过 `LoadDependencies()` 递归加载
3. **延迟创建**: GPU 资源延迟创建，只在需要时创建
4. **每帧更新**: 渲染参数每帧更新并上传到 GPU
5. **资源缓存**: CPU 资源在 ResourceManager 中缓存，GPU 资源在 RenderResourceManager 中管理
6. **生命周期**: CPU 资源通过引用计数管理，GPU 资源通过 RenderResourceManager 管理
7. **数据封装**: Resource 类通过 `GetRenderData()` 方法封装 GPU 资源，不直接暴露 RenderGeometry/ShadingMaterial
8. **Staging Buffer**: 使用 Staging Buffer 上传数据到 device-local 内存，提升性能

---

## 相关文件

### 资源加载
- `src/Resources/MeshLoader.cpp` - Mesh 加载（Assimp）
- `src/Resources/MaterialLoader.cpp` - Material 加载（XML）
- `src/Resources/ModelLoader.cpp` - Model 加载（XML）
- `src/Resources/ResourceXMLParser.cpp` - XML 解析和保存
- `src/Resources/ResourceProvider.cpp` - 资源管理器

### 资源类（CPU 端）
- `src/Resources/MeshResource.cpp` - Mesh 资源
- `src/Resources/MaterialResource.cpp` - Material 资源
- `src/Resources/ModelResource.cpp` - Model 资源
- `src/Resources/TextureResource.cpp` - Texture 资源
- `src/Resources/ModelComponent.cpp` - Model Component（Entity 组件）

### 渲染资源（GPU 端）
- `src/Renderer/RenderGeometry.cpp` - GPU 几何资源（顶点/索引缓冲区）
- `src/Renderer/ShadingMaterial.cpp` - GPU 材质资源（Pipeline/DescriptorSet）
- `src/Renderer/RenderResourceManager.cpp` - 渲染资源管理器
- `src/Renderer/MaterialDescriptorManager.cpp` - Descriptor Set 管理

### 渲染流程
- `src/Renderer/SceneRenderer.cpp` - 场景渲染（Entity → RenderItem）
- `src/Renderer/RenderBatch.cpp` - 渲染批处理（RenderItem 分组）
- `src/Renderer/CommandRecorder.cpp` - 命令记录（RenderCommand → GPU Command）
- `src/Renderer/FrameGraph.cpp` - 帧图执行（Pass 管理和资源分配）
- `src/Renderer/RenderContext.cpp` - 渲染上下文（整体流程协调）

### 数据转换
- `src/Renderer/RenderParameterCollector.cpp` - 参数收集器
- `src/Device/VulkanRHIWrappers.cpp` - Vulkan RHI 实现
- `src/Device/VulkanDevice.cpp` - Vulkan 设备接口
