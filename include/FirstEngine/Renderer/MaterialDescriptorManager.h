#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace FirstEngine {
    namespace Renderer {

        // Forward declarations
        class ShadingMaterial;

        // ============================================================================
        // MaterialDescriptorManager - 专门处理 Descriptor 和 Binding 的设备相关逻辑
        // ============================================================================
        // 职责：
        // 1. 管理 Descriptor Set Layouts 的创建和销毁
        // 2. 管理 Descriptor Pool 的创建和销毁
        // 3. 分配和管理 Descriptor Sets
        // 4. 更新 Descriptor Sets 的绑定（Uniform Buffers、Textures）
        // 5. 提供 Descriptor Set 的访问接口
        //
        // ShadingMaterial 专注于：
        // - 保存 Shader 参数（CPU 端数据）
        // - 记录 CPU 到 GPU 的参数传递
        // - 不直接与 RHI 交互
        // ============================================================================
        class FE_RENDERER_API MaterialDescriptorManager {
        public:
            MaterialDescriptorManager();
            ~MaterialDescriptorManager();

            // 禁用拷贝和赋值
            MaterialDescriptorManager(const MaterialDescriptorManager&) = delete;
            MaterialDescriptorManager& operator=(const MaterialDescriptorManager&) = delete;

            // 初始化：从 ShadingMaterial 的资源信息创建 descriptor sets
            // material: ShadingMaterial 实例，提供 uniform buffers 和 texture bindings 信息
            // device: RHI 设备，用于创建 GPU 资源
            // Returns: true if successful, false otherwise
            bool Initialize(ShadingMaterial* material, RHI::IDevice* device);

            // 清理所有资源
            void Cleanup(RHI::IDevice* device);

            // 更新 descriptor set 绑定（当 uniform buffer 或 texture 改变时调用）
            // material: ShadingMaterial 实例，提供最新的资源信息
            // device: RHI 设备
            // Returns: true if successful, false otherwise
            bool UpdateBindings(ShadingMaterial* material, RHI::IDevice* device);

            // 获取 descriptor set（用于渲染时绑定）
            // setIndex: Descriptor set 索引
            // Returns: Descriptor set handle，如果不存在则返回 nullptr
            RHI::DescriptorSetHandle GetDescriptorSet(uint32_t setIndex) const;

            // 获取 descriptor set layout（用于创建 pipeline layout）
            // setIndex: Descriptor set 索引
            // Returns: Descriptor set layout handle，如果不存在则返回 nullptr
            RHI::DescriptorSetLayoutHandle GetDescriptorSetLayout(uint32_t setIndex) const;

            // 获取所有 descriptor set layouts（用于创建 pipeline）
            // Returns: 所有 descriptor set layout handles 的列表（按 set 索引排序）
            std::vector<RHI::DescriptorSetLayoutHandle> GetAllDescriptorSetLayouts() const;

            // 检查是否已初始化
            bool IsInitialized() const { return m_Initialized; }

        private:
            // 从 ShadingMaterial 创建 descriptor set layouts
            bool CreateDescriptorSetLayouts(ShadingMaterial* material, RHI::IDevice* device);

            // 创建 descriptor pool
            bool CreateDescriptorPool(ShadingMaterial* material, RHI::IDevice* device);

            // 分配 descriptor sets
            bool AllocateDescriptorSets(RHI::IDevice* device);

            // 更新 descriptor set 绑定（写入 uniform buffers 和 textures）
            void WriteDescriptorSets(ShadingMaterial* material, RHI::IDevice* device);

            // 内部状态
            bool m_Initialized = false;
            RHI::IDevice* m_Device = nullptr;

            // Descriptor set layouts (one per set index)
            std::unordered_map<uint32_t, RHI::DescriptorSetLayoutHandle> m_DescriptorSetLayouts;

            // Descriptor pool (shared for all descriptor sets)
            RHI::DescriptorPoolHandle m_DescriptorPool = nullptr;

            // Descriptor sets (one per set index)
            std::unordered_map<uint32_t, RHI::DescriptorSetHandle> m_DescriptorSets;
        };

    } // namespace Renderer
} // namespace FirstEngine
