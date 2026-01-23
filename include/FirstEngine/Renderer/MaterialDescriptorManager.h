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
        // MaterialDescriptorManager - Handles device-specific logic for Descriptors and Bindings
        // ============================================================================
        // Responsibilities:
        // 1. Manage creation and destruction of Descriptor Set Layouts
        // 2. Manage creation and destruction of Descriptor Pool
        // 3. Allocate and manage Descriptor Sets
        // 4. Update Descriptor Set bindings (Uniform Buffers, Textures)
        // 5. Provide access interface for Descriptor Sets
        //
        // ShadingMaterial focuses on:
        // - Storing Shader parameters (CPU-side data)
        // - Recording CPU to GPU parameter transfer
        // - Not directly interacting with RHI
        // ============================================================================
        class FE_RENDERER_API MaterialDescriptorManager {
        public:
            MaterialDescriptorManager();
            ~MaterialDescriptorManager();

            // Disable copy and assignment
            MaterialDescriptorManager(const MaterialDescriptorManager&) = delete;
            MaterialDescriptorManager& operator=(const MaterialDescriptorManager&) = delete;

            // Initialize: Create descriptor sets from ShadingMaterial's resource information
            // material: ShadingMaterial instance, provides uniform buffers and texture bindings information
            // device: RHI device, used to create GPU resources
            // Returns: true if successful, false otherwise
            bool Initialize(ShadingMaterial* material, RHI::IDevice* device);

            // Cleanup all resources
            void Cleanup(RHI::IDevice* device);

            // Update descriptor set bindings (called when uniform buffer or texture changes)
            // material: ShadingMaterial instance, provides latest resource information
            // device: RHI device
            // Returns: true if successful, false otherwise
            bool UpdateBindings(ShadingMaterial* material, RHI::IDevice* device);

            // Get descriptor set (for binding during rendering)
            // setIndex: Descriptor set index
            // Returns: Descriptor set handle, returns nullptr if doesn't exist
            RHI::DescriptorSetHandle GetDescriptorSet(uint32_t setIndex) const;

            // Get descriptor set layout (for creating pipeline layout)
            // setIndex: Descriptor set index
            // Returns: Descriptor set layout handle, returns nullptr if doesn't exist
            RHI::DescriptorSetLayoutHandle GetDescriptorSetLayout(uint32_t setIndex) const;

            // Get all descriptor set layouts (for creating pipeline)
            // Returns: List of all descriptor set layout handles (sorted by set index)
            std::vector<RHI::DescriptorSetLayoutHandle> GetAllDescriptorSetLayouts() const;

            // Check if initialized
            bool IsInitialized() const { return m_Initialized; }

        private:
            // Create descriptor set layouts from ShadingMaterial
            bool CreateDescriptorSetLayouts(ShadingMaterial* material, RHI::IDevice* device);

            // Create descriptor pool
            bool CreateDescriptorPool(ShadingMaterial* material, RHI::IDevice* device);

            // Allocate descriptor sets
            bool AllocateDescriptorSets(RHI::IDevice* device);

            // Update descriptor set bindings (write uniform buffers and textures)
            void WriteDescriptorSets(ShadingMaterial* material, RHI::IDevice* device);

            // Internal state
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
