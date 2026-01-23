# DescriptorSet 从材质构造到 VulkanDevice 的完整流程

## 概述

本文档详细梳理了 DescriptorSet 从 Shader 反射数据开始，经过材质解析、DescriptorSetLayout 创建、DescriptorSet 分配，最终传递到 VulkanDevice 并更新到 GPU 的完整流程。

---

## 流程图概览

```
Shader (HLSL/GLSL)
    ↓ [编译 + 反射]
ShaderReflection (uniform_buffers, samplers, images)
    ↓ [ParseShaderReflection]
ShadingMaterial (m_UniformBuffers, m_TextureBindings)
    ↓ [MaterialDescriptorManager::Initialize]
MaterialDescriptorManager
    ├─ CreateDescriptorSetLayouts → RHI::DescriptorSetLayoutDescription
    ├─ CreateDescriptorPool → RHI::DescriptorPoolHandle
    ├─ AllocateDescriptorSets → RHI::DescriptorSetHandle
    └─ WriteDescriptorSets → RHI::DescriptorWrite
        ↓ [VulkanDevice API]
VulkanDevice
    ├─ CreateDescriptorSetLayout → VkDescriptorSetLayout
    ├─ CreateDescriptorPool → VkDescriptorPool
    ├─ AllocateDescriptorSets → VkDescriptorSet
    └─ UpdateDescriptorSets → vkUpdateDescriptorSets
        ↓ [Vulkan API]
GPU DescriptorSet (已绑定资源)
```

---

## 详细流程

### 阶段 1: Shader 编译与反射

**位置**: `ShaderCompiler::GetReflection()`

**过程**:
1. Shader 源码（HLSL/GLSL）编译为 SPIR-V
2. 使用 SPIRV-Cross 解析 SPIR-V，提取资源信息
3. 生成 `ShaderReflection` 结构，包含：
   - `uniform_buffers`: Uniform Buffer 信息（set, binding, size, name）
   - `samplers`: Sampler 信息（set, binding, name）
   - `images`: Image 信息（set, binding, name）

**关键数据结构**:
```cpp
struct ShaderReflection {
    std::vector<UniformBuffer> uniform_buffers;  // set, binding, size, name
    std::vector<ShaderResource> samplers;        // set, binding, name
    std::vector<ShaderResource> images;          // set, binding, name
};
```

---

### 阶段 2: ShadingMaterial 解析 ShaderReflection

**位置**: `ShadingMaterial::ParseShaderReflection()`

**过程**:
1. 从 `ShaderReflection` 中提取 Uniform Buffer 信息
   - 创建 `UniformBufferBinding` 结构
   - 存储 set, binding, name, size
   - 分配 CPU 端数据缓冲区 (`binding.data`)

2. 从 `ShaderReflection` 中提取 Texture 信息
   - 遍历 `reflection.samplers`（Combined Image Sampler）
   - 遍历 `reflection.images`（单独的 Texture Image）
   - 创建 `TextureBinding` 结构
   - 存储 set, binding, name（texture 指针在运行时设置）

**关键数据结构**:
```cpp
// ShadingMaterial 中的成员
std::vector<UniformBufferBinding> m_UniformBuffers;  // set, binding, name, size, buffer, data
std::vector<TextureBinding> m_TextureBindings;        // set, binding, name, texture
```

**代码位置**: `src/Renderer/ShadingMaterial.cpp:561-727`

---

### 阶段 3: MaterialDescriptorManager 初始化

**位置**: `MaterialDescriptorManager::Initialize()`

**调用时机**: `ShadingMaterial::DoCreate()` 中调用

**过程**:
1. **CreateDescriptorSetLayouts()** - 创建 DescriptorSetLayout
   - 从 `material->GetUniformBuffers()` 收集 Uniform Buffer bindings
   - 从 `material->GetTextureBindings()` 收集 Texture bindings
   - 按 set 分组，创建 `RHI::DescriptorSetLayoutDescription`
   - 调用 `device->CreateDescriptorSetLayout()` 创建布局

2. **CreateDescriptorPool()** - 创建 DescriptorPool
   - 统计 Uniform Buffer 和 Texture 的数量
   - 创建 `RHI::DescriptorPoolHandle`
   - 调用 `device->CreateDescriptorPool()` 创建池

3. **AllocateDescriptorSets()** - 分配 DescriptorSet
   - 从 DescriptorPool 中分配 DescriptorSet
   - 调用 `device->AllocateDescriptorSets()` 分配
   - 存储 `RHI::DescriptorSetHandle` 到 `m_DescriptorSets`

4. **WriteDescriptorSets()** - 写入初始绑定
   - 为每个 Uniform Buffer 创建 `RHI::DescriptorWrite`
   - 为每个 Texture 创建 `RHI::DescriptorWrite`
   - 调用 `device->UpdateDescriptorSets()` 更新

**代码位置**: `src/Renderer/MaterialDescriptorManager.cpp:22-348`

---

### 阶段 4: VulkanDevice 创建 DescriptorSetLayout

**位置**: `VulkanDevice::CreateDescriptorSetLayout()`

**过程**:
1. 接收 `RHI::DescriptorSetLayoutDescription`
2. 转换每个 `RHI::DescriptorBinding` 为 `VkDescriptorSetLayoutBinding`
   - `binding.binding` → `vkBinding.binding`
   - `binding.type` → `vkBinding.descriptorType` (通过 `ConvertDescriptorType`)
   - `binding.count` → `vkBinding.descriptorCount`
   - `binding.stageFlags` → `vkBinding.stageFlags` (通过 `ConvertShaderStageFlags`)

3. 创建 `VkDescriptorSetLayoutCreateInfo`
4. 调用 `vkCreateDescriptorSetLayout()` 创建 Vulkan 对象
5. 返回 `RHI::DescriptorSetLayoutHandle` (实际上是 `VkDescriptorSetLayout` 的指针)

**代码位置**: `src/Device/VulkanDevice.cpp:820-853`

---

### 阶段 5: VulkanDevice 创建 DescriptorPool

**位置**: `VulkanDevice::CreateDescriptorPool()`

**过程**:
1. 接收 `maxSets` 和 `poolSizes` (DescriptorType 和数量的对)
2. 转换 `RHI::DescriptorType` 为 `VkDescriptorType`
3. 创建 `VkDescriptorPoolSize` 数组
4. 创建 `VkDescriptorPoolCreateInfo`
5. 调用 `vkCreateDescriptorPool()` 创建 Vulkan 对象
6. 返回 `RHI::DescriptorPoolHandle` (实际上是 `VkDescriptorPool` 的指针)

**代码位置**: `src/Device/VulkanDevice.cpp:869-902`

---

### 阶段 6: VulkanDevice 分配 DescriptorSet

**位置**: `VulkanDevice::AllocateDescriptorSets()`

**过程**:
1. 接收 `RHI::DescriptorPoolHandle` 和 `std::vector<RHI::DescriptorSetLayoutHandle>`
2. 转换 `RHI::DescriptorSetLayoutHandle` 为 `VkDescriptorSetLayout`
3. 创建 `VkDescriptorSetAllocateInfo`
   - `descriptorPool`: 从 pool handle 转换
   - `descriptorSetCount`: layouts 的数量
   - `pSetLayouts`: 转换后的 layout 数组
4. 调用 `vkAllocateDescriptorSets()` 分配 DescriptorSet
5. 返回 `std::vector<RHI::DescriptorSetHandle>` (实际上是 `VkDescriptorSet` 的指针数组)

**代码位置**: `src/Device/VulkanDevice.cpp:918-958`

---

### 阶段 7: VulkanDevice 更新 DescriptorSet

**位置**: `VulkanDevice::UpdateDescriptorSets()`

**过程**:
1. 接收 `std::vector<RHI::DescriptorWrite>`
2. 对每个 `RHI::DescriptorWrite`:
   - 转换 `write.dstSet` 为 `VkDescriptorSet`
   - 转换 `write.descriptorType` 为 `VkDescriptorType`
   - 处理 Buffer Info:
     - 从 `RHI::IBuffer` 获取 `VkBuffer` (通过 `VulkanBuffer::GetVkBuffer()`)
     - 创建 `VkDescriptorBufferInfo`
   - 处理 Image Info:
     - 从 `RHI::IImageView` 获取 `VkImageView` (通过 `VulkanImageView::GetVkImageView()`)
     - 如果 ImageView 为空，尝试从 Image 创建默认 ImageView
     - 创建 `VkDescriptorImageInfo`
   - 创建 `VkWriteDescriptorSet`
3. 调用 `vkUpdateDescriptorSets()` 更新所有 DescriptorSet

**代码位置**: `src/Device/VulkanDevice.cpp:983-1113`

---

## 数据转换映射

### RHI → Vulkan 类型转换

| RHI 类型 | Vulkan 类型 | 转换函数 |
|---------|-----------|---------|
| `RHI::DescriptorSetLayoutHandle` | `VkDescriptorSetLayout` | `reinterpret_cast<VkDescriptorSetLayout>(handle)` |
| `RHI::DescriptorPoolHandle` | `VkDescriptorPool` | `reinterpret_cast<VkDescriptorPool>(handle)` |
| `RHI::DescriptorSetHandle` | `VkDescriptorSet` | `reinterpret_cast<VkDescriptorSet>(handle)` |
| `RHI::DescriptorType::UniformBuffer` | `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` | `Device::ConvertDescriptorType()` |
| `RHI::DescriptorType::CombinedImageSampler` | `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` | `Device::ConvertDescriptorType()` |
| `RHI::IBuffer*` | `VkBuffer` | `static_cast<VulkanBuffer*>(buffer)->GetVkBuffer()` |
| `RHI::IImageView*` | `VkImageView` | `static_cast<VulkanImageView*>(imageView)->GetVkImageView()` |

---

## 关键数据结构

### 1. ShaderReflection (Shader 反射数据)
```cpp
struct ShaderReflection {
    std::vector<UniformBuffer> uniform_buffers;  // set, binding, size, name
    std::vector<ShaderResource> samplers;        // set, binding, name
    std::vector<ShaderResource> images;          // set, binding, name
};
```

### 2. ShadingMaterial 中的绑定信息
```cpp
struct UniformBufferBinding {
    uint32_t set;
    uint32_t binding;
    std::string name;
    uint32_t size;
    std::unique_ptr<RHI::IBuffer> buffer;  // GPU buffer
    std::vector<uint8_t> data;             // CPU data
};

struct TextureBinding {
    uint32_t set;
    uint32_t binding;
    std::string name;
    RHI::IImage* texture;  // Texture reference (not owned)
};
```

### 3. RHI 层描述符结构
```cpp
struct DescriptorBinding {
    uint32_t binding;
    DescriptorType type;
    uint32_t count;
    ShaderStage stageFlags;
};

struct DescriptorSetLayoutDescription {
    std::vector<DescriptorBinding> bindings;
};

struct DescriptorWrite {
    DescriptorSetHandle dstSet;
    uint32_t dstBinding;
    uint32_t dstArrayElement;
    DescriptorType descriptorType;
    std::vector<DescriptorBufferInfo> bufferInfo;
    std::vector<DescriptorImageInfo> imageInfo;
};
```

---

## 调用链示例

### 初始化流程
```
ShadingMaterial::DoCreate()
  └─ MaterialDescriptorManager::Initialize()
      ├─ CreateDescriptorSetLayouts()
      │   └─ device->CreateDescriptorSetLayout()
      │       └─ VulkanDevice::CreateDescriptorSetLayout()
      │           └─ vkCreateDescriptorSetLayout()
      ├─ CreateDescriptorPool()
      │   └─ device->CreateDescriptorPool()
      │       └─ VulkanDevice::CreateDescriptorPool()
      │           └─ vkCreateDescriptorPool()
      ├─ AllocateDescriptorSets()
      │   └─ device->AllocateDescriptorSets()
      │       └─ VulkanDevice::AllocateDescriptorSets()
      │           └─ vkAllocateDescriptorSets()
      └─ WriteDescriptorSets()
          └─ device->UpdateDescriptorSets()
              └─ VulkanDevice::UpdateDescriptorSets()
                  └─ vkUpdateDescriptorSets()
```

### 运行时更新流程
```
ShadingMaterial::UpdateTexture() / UpdateUniformBuffer()
  └─ MaterialDescriptorManager::UpdateBindings()
      └─ WriteDescriptorSets()
          └─ device->UpdateDescriptorSets()
              └─ VulkanDevice::UpdateDescriptorSets()
                  └─ vkUpdateDescriptorSets()
```

---

## 注意事项

1. **生命周期管理**:
   - DescriptorSetLayout 在 MaterialDescriptorManager 析构时销毁
   - DescriptorPool 在 MaterialDescriptorManager 析构时销毁
   - DescriptorSet 在 MaterialDescriptorManager::Cleanup() 时释放

2. **资源绑定**:
   - Uniform Buffer 必须在 `DoCreate()` 时创建 GPU Buffer
   - Texture 可以在运行时通过 `SetTexture()` 设置
   - ImageView 在 `WriteDescriptorSets()` 时通过 `texture->CreateImageView()` 创建

3. **验证**:
   - `VulkanDevice::UpdateDescriptorSets()` 中有大量验证逻辑
   - 检查 ImageView 是否为 `VK_NULL_HANDLE`
   - 检查 Buffer 是否为 `VK_NULL_HANDLE`
   - 检查 descriptorCount 是否为 0

4. **错误处理**:
   - 所有 Vulkan API 调用都有错误检查
   - 失败时返回 `nullptr` 或空向量
   - 使用 `std::cerr` 输出错误信息

---

## 相关文件

- **Shader 反射**: `include/FirstEngine/Shader/ShaderCompiler.h`, `src/Shader/ShaderCompiler.cpp`
- **材质定义**: `include/FirstEngine/Renderer/ShadingMaterial.h`, `src/Renderer/ShadingMaterial.cpp`
- **描述符管理**: `include/FirstEngine/Renderer/MaterialDescriptorManager.h`, `src/Renderer/MaterialDescriptorManager.cpp`
- **Vulkan 设备**: `include/FirstEngine/Device/VulkanDevice.h`, `src/Device/VulkanDevice.cpp`
- **RHI 类型**: `include/FirstEngine/RHI/Types.h`, `include/FirstEngine/RHI/IDevice.h`
