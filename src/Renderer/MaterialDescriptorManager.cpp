#include "FirstEngine/Renderer/MaterialDescriptorManager.h"
#include <map>
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/RHI/IDevice.h"
#include <iostream>
#include <algorithm>

namespace FirstEngine {
    namespace Renderer {

        MaterialDescriptorManager::MaterialDescriptorManager() {
        }

        MaterialDescriptorManager::~MaterialDescriptorManager() {
            // Cleanup should be called explicitly before destruction
            // But we'll do a safety cleanup here if needed
            if (m_Initialized && m_Device) {
                Cleanup(m_Device);
            }
        }

        bool MaterialDescriptorManager::Initialize(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device) {
                return false;
            }

            if (m_Initialized) {
                // Already initialized, cleanup first
                Cleanup(device);
            }

            m_Device = device;

            // Create descriptor set layouts
            if (!CreateDescriptorSetLayouts(material, device)) {
                return false;
            }

            // Create descriptor pool
            if (!CreateDescriptorPool(material, device)) {
                Cleanup(device);
                return false;
            }

            // Allocate descriptor sets
            if (!AllocateDescriptorSets(device)) {
                Cleanup(device);
                return false;
            }

            // Write initial bindings
            WriteDescriptorSets(material, device);

            m_Initialized = true;
            return true;
        }

        void MaterialDescriptorManager::Cleanup(RHI::IDevice* device) {
            if (!device) {
                return;
            }

            // Free descriptor sets
            if (m_DescriptorPool && !m_DescriptorSets.empty()) {
                std::vector<RHI::DescriptorSetHandle> sets;
                for (const auto& [_, set] : m_DescriptorSets) {
                    sets.push_back(set);
                }
                device->FreeDescriptorSets(m_DescriptorPool, sets);
            }

            // Destroy descriptor pool
            if (m_DescriptorPool) {
                device->DestroyDescriptorPool(m_DescriptorPool);
                m_DescriptorPool = nullptr;
            }

            // Destroy descriptor set layouts
            for (const auto& [_, layout] : m_DescriptorSetLayouts) {
                device->DestroyDescriptorSetLayout(layout);
            }
            m_DescriptorSetLayouts.clear();

            m_DescriptorSets.clear();
            m_Initialized = false;
            m_Device = nullptr;
        }

        bool MaterialDescriptorManager::UpdateBindings(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device || !m_Initialized) {
                return false;
            }

            WriteDescriptorSets(material, device);
            return true;
        }

        RHI::DescriptorSetHandle MaterialDescriptorManager::GetDescriptorSet(uint32_t setIndex) const {
            auto it = m_DescriptorSets.find(setIndex);
            return (it != m_DescriptorSets.end()) ? it->second : nullptr;
        }

        RHI::DescriptorSetLayoutHandle MaterialDescriptorManager::GetDescriptorSetLayout(uint32_t setIndex) const {
            auto it = m_DescriptorSetLayouts.find(setIndex);
            return (it != m_DescriptorSetLayouts.end()) ? it->second : nullptr;
        }

        std::vector<RHI::DescriptorSetLayoutHandle> MaterialDescriptorManager::GetAllDescriptorSetLayouts() const {
            std::vector<RHI::DescriptorSetLayoutHandle> layouts;
            layouts.reserve(m_DescriptorSetLayouts.size());

            // Sort by set index to ensure consistent ordering
            std::vector<uint32_t> setIndices;
            for (const auto& [setIndex, _] : m_DescriptorSetLayouts) {
                setIndices.push_back(setIndex);
            }
            std::sort(setIndices.begin(), setIndices.end());

            for (uint32_t setIndex : setIndices) {
                layouts.push_back(m_DescriptorSetLayouts.at(setIndex));
            }

            return layouts;
        }

        bool MaterialDescriptorManager::CreateDescriptorSetLayouts(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device) {
                return false;
            }

            // Group resources by descriptor set and create descriptor set layouts
            std::map<uint32_t, RHI::DescriptorSetLayoutDescription> setLayouts;

            // Collect uniform buffer bindings by set
            const auto& uniformBuffers = material->GetUniformBuffers();
            for (const auto& ub : uniformBuffers) {
                RHI::DescriptorBinding binding;
                binding.binding = ub.binding;
                binding.type = RHI::DescriptorType::UniformBuffer;
                binding.count = 1;
                // Determine shader stages that use this buffer (default to all graphics stages)
                binding.stageFlags = static_cast<RHI::ShaderStage>(
                    static_cast<uint32_t>(RHI::ShaderStage::Vertex) |
                    static_cast<uint32_t>(RHI::ShaderStage::Fragment)
                );
                setLayouts[ub.set].bindings.push_back(binding);
            }

            // Collect texture bindings by set
            const auto& textureBindings = material->GetTextureBindings();
            for (const auto& tb : textureBindings) {
                RHI::DescriptorBinding binding;
                binding.binding = tb.binding;
                binding.type = RHI::DescriptorType::CombinedImageSampler; // Default to combined sampler
                binding.count = 1;
                // Determine shader stages that use this texture (default to fragment stage)
                binding.stageFlags = RHI::ShaderStage::Fragment;
                setLayouts[tb.set].bindings.push_back(binding);
            }

            // Create descriptor set layouts for each set
            for (const auto& [setIndex, layoutDesc] : setLayouts) {
                RHI::DescriptorSetLayoutHandle layout = device->CreateDescriptorSetLayout(layoutDesc);
                if (!layout) {
                    std::cerr << "Failed to create descriptor set layout for set " << setIndex << std::endl;
                    // Cleanup already created layouts
                    for (const auto& [idx, handle] : m_DescriptorSetLayouts) {
                        device->DestroyDescriptorSetLayout(handle);
                    }
                    m_DescriptorSetLayouts.clear();
                    return false;
                }
                m_DescriptorSetLayouts[setIndex] = layout;
            }

            return true;
        }

        bool MaterialDescriptorManager::CreateDescriptorPool(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device) {
                return false;
            }

            // Calculate pool sizes based on resource counts
            std::vector<std::pair<RHI::DescriptorType, uint32_t>> poolSizes;

            // Count uniform buffers
            const auto& uniformBuffers = material->GetUniformBuffers();
            uint32_t uniformBufferCount = static_cast<uint32_t>(uniformBuffers.size());
            if (uniformBufferCount > 0) {
                poolSizes.push_back({RHI::DescriptorType::UniformBuffer, uniformBufferCount});
            }

            // Count textures
            const auto& textureBindings = material->GetTextureBindings();
            uint32_t textureCount = static_cast<uint32_t>(textureBindings.size());
            if (textureCount > 0) {
                poolSizes.push_back({RHI::DescriptorType::CombinedImageSampler, textureCount});
            }

            if (poolSizes.empty()) {
                return true; // No resources, no pool needed
            }

            // Allocate enough sets for all sets we need
            uint32_t maxSets = static_cast<uint32_t>(m_DescriptorSetLayouts.size());
            m_DescriptorPool = device->CreateDescriptorPool(maxSets, poolSizes);
            if (!m_DescriptorPool) {
                std::cerr << "Failed to create descriptor pool!" << std::endl;
                return false;
            }

            return true;
        }

        bool MaterialDescriptorManager::AllocateDescriptorSets(RHI::IDevice* device) {
            if (!device || m_DescriptorSetLayouts.empty() || !m_DescriptorPool) {
                return false;
            }

            // Collect layouts in order
            std::vector<RHI::DescriptorSetLayoutHandle> layoutsToAllocate;
            std::vector<uint32_t> setIndices;
            for (const auto& [setIndex, layout] : m_DescriptorSetLayouts) {
                setIndices.push_back(setIndex);
            }
            std::sort(setIndices.begin(), setIndices.end());

            for (uint32_t setIndex : setIndices) {
                layoutsToAllocate.push_back(m_DescriptorSetLayouts[setIndex]);
            }

            // Allocate descriptor sets from pool
            std::vector<RHI::DescriptorSetHandle> allocatedSets = 
                device->AllocateDescriptorSets(m_DescriptorPool, layoutsToAllocate);

            if (allocatedSets.size() != setIndices.size()) {
                std::cerr << "Failed to allocate all descriptor sets!" << std::endl;
                return false;
            }

            // Store allocated descriptor sets
            for (size_t i = 0; i < setIndices.size(); ++i) {
                m_DescriptorSets[setIndices[i]] = allocatedSets[i];
            }

            return true;
        }

        void MaterialDescriptorManager::WriteDescriptorSets(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device) {
                return;
            }

            std::vector<RHI::DescriptorWrite> writes;

            // Write uniform buffers to descriptor sets
            const auto& uniformBuffers = material->GetUniformBuffers();
            for (const auto& ub : uniformBuffers) {
                if (!ub.buffer) {
                    continue;
                }

                RHI::DescriptorSetHandle set = GetDescriptorSet(ub.set);
                if (!set) {
                    continue;
                }

                RHI::DescriptorWrite write;
                write.dstSet = set;
                write.dstBinding = ub.binding;
                write.dstArrayElement = 0;
                write.descriptorType = RHI::DescriptorType::UniformBuffer;

                RHI::DescriptorBufferInfo bufferInfo;
                bufferInfo.buffer = ub.buffer.get();
                bufferInfo.offset = 0;
                bufferInfo.range = ub.size; // Use entire buffer
                write.bufferInfo.push_back(bufferInfo);

                writes.push_back(write);
            }

            // Write textures to descriptor sets
            const auto& textureBindings = material->GetTextureBindings();
            for (const auto& tb : textureBindings) {
                if (!tb.texture) {
                    continue;
                }

                RHI::DescriptorSetHandle set = GetDescriptorSet(tb.set);
                if (!set) {
                    continue;
                }

                RHI::DescriptorWrite write;
                write.dstSet = set;
                write.dstBinding = tb.binding;
                write.dstArrayElement = 0;
                write.descriptorType = RHI::DescriptorType::CombinedImageSampler;

                RHI::DescriptorImageInfo imageInfo;
                imageInfo.image = tb.texture;
                // Get image view from texture
                if (tb.texture) {
                    imageInfo.imageView = tb.texture->CreateImageView();
                } else {
                    imageInfo.imageView = nullptr;
                }
                imageInfo.sampler = nullptr; // TODO: Create/get sampler (for now, use default/null)
                write.imageInfo.push_back(imageInfo);

                writes.push_back(write);
            }

            // Update descriptor sets
            if (!writes.empty()) {
                device->UpdateDescriptorSets(writes);
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
