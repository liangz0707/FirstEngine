#include "FirstEngine/Renderer/MaterialDescriptorManager.h"
#include <map>
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include <iostream>
#include <algorithm>
#include <utility> // for std::pair

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
            m_LastTexturePointers.clear(); // Clear texture pointer cache
            m_UpdatedThisFrame.clear(); // Clear per-frame update tracking
            m_CurrentFrame = 0;
            m_Initialized = false;
            m_Device = nullptr;
        }

        bool MaterialDescriptorManager::UpdateBindings(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device || !m_Initialized) {
                return false;
            }

            // Skip uniform buffer bindings in UpdateBindings to avoid validation warnings.
            // Uniform buffer content is updated via UpdateData() in DoUpdate(), and buffer pointers
            // typically don't change between frames. Initial binding is done in Initialize().
            // 
            // IMPORTANT: We only update texture bindings if:
            // 1. Texture pointer has changed (checked via cache)
            // 2. Descriptor set hasn't been updated this frame (to avoid updating sets in use by command buffers)
            //
            // NOTE: This function may be called multiple times per frame if:
            // - Multiple entities share the same material (each needs per-object data updated)
            // - The same entity has multiple components using the same material
            // - Child entities also use the same material
            // The per-frame tracking (m_UpdatedThisFrame) prevents duplicate descriptor set updates
            WriteDescriptorSets(material, device, false);
            return true;
        }
        
        void MaterialDescriptorManager::BeginFrame() {
            // Clear per-frame update tracking at the start of each frame
            // This allows descriptor sets to be updated again in the new frame
            m_UpdatedThisFrame.clear();
            m_CurrentFrame++;
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

            // Get shader reflection to verify resource types match
            const auto& shaderReflection = material->GetShaderReflection();
            
            // Debug: Print what we're about to create
            std::cerr << "MaterialDescriptorManager::CreateDescriptorSetLayouts: Starting layout creation..." << std::endl;
            std::cerr << "  Uniform buffers from material: " << material->GetUniformBuffers().size() << std::endl;
            std::cerr << "  Texture bindings from material: " << material->GetTextureBindings().size() << std::endl;
            std::cerr << "  Shader reflection - sampled_images: " << shaderReflection.sampled_images.size() 
                      << ", separate_images: " << shaderReflection.separate_images.size() 
                      << ", separate_samplers: " << shaderReflection.separate_samplers.size() << std::endl;

            // Group resources by descriptor set and create descriptor set layouts
            std::map<uint32_t, RHI::DescriptorSetLayoutDescription> setLayouts;
            // Track bindings per set to detect conflicts
            std::map<uint32_t, std::map<uint32_t, RHI::DescriptorType>> setBindingMap;

            // Collect uniform buffer bindings by set
            const auto& uniformBuffers = material->GetUniformBuffers();
            std::cerr << "  Processing " << uniformBuffers.size() << " uniform buffers..." << std::endl;
            for (const auto& ub : uniformBuffers) {
                std::cerr << "    - Uniform buffer '" << ub.name << "' at Set " << ub.set << ", Binding " << ub.binding << std::endl;
                // Check for binding conflicts
                auto& bindings = setBindingMap[ub.set];
                if (bindings.find(ub.binding) != bindings.end()) {
                    std::cerr << "Error: MaterialDescriptorManager::CreateDescriptorSetLayouts: "
                              << "Binding conflict detected! Set " << ub.set << ", Binding " << ub.binding
                              << " is already used by type " << static_cast<int>(bindings[ub.binding])
                              << ", trying to add UniformBuffer" << std::endl;
                    return false;
                }
                bindings[ub.binding] = RHI::DescriptorType::UniformBuffer;

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
            // IMPORTANT: Use the descriptor type from TextureBinding, which is determined from shader reflection
            // This ensures we use the correct type (COMBINED_IMAGE_SAMPLER, SAMPLED_IMAGE, or SAMPLER)
            const auto& textureBindings = material->GetTextureBindings();
            std::cerr << "  Processing " << textureBindings.size() << " texture bindings..." << std::endl;
            
            // First pass: collect all texture bindings to identify separate sampler/image pairs
            // Separate samplers can share bindings with separate images (they're used together)
            // But we need to track which bindings are used by separate samplers vs separate images
            std::map<uint32_t, std::map<uint32_t, std::vector<std::pair<RHI::DescriptorType, std::string>>>> textureBindingTypes;
            for (const auto& tb : textureBindings) {
                textureBindingTypes[tb.set][tb.binding].push_back({tb.descriptorType, tb.name});
            }
            
            for (const auto& tb : textureBindings) {
                std::cerr << "    - Texture binding '" << tb.name << "' at Set " << tb.set << ", Binding " << tb.binding 
                          << ", Type: " << static_cast<int>(tb.descriptorType) << std::endl;
                
                // Check for binding conflicts with uniform buffers
                // Note: Separate samplers and separate images can share bindings (they're used together)
                // But they cannot conflict with uniform buffers or other non-texture resources
                auto& bindings = setBindingMap[tb.set];
                if (bindings.find(tb.binding) != bindings.end()) {
                    // Check if the existing binding is a texture type that can coexist
                    RHI::DescriptorType existingType = bindings[tb.binding];
                    
                    // Separate samplers (DescriptorType::Sampler) and separate images (SampledImage) can coexist
                    // This is because separate samplers are used with separate images in Vulkan
                    // However, they cannot conflict with uniform buffers
                    if (existingType == RHI::DescriptorType::UniformBuffer) {
                        std::cerr << "Error: MaterialDescriptorManager::CreateDescriptorSetLayouts: "
                                  << "Binding conflict detected! Set " << tb.set << ", Binding " << tb.binding
                                  << " is already used by UniformBuffer, trying to add texture type " 
                                  << static_cast<int>(tb.descriptorType) << " (" << tb.name << ")" << std::endl;
                        std::cerr << "  This is a shader definition issue - uniform buffers and textures cannot share the same binding." << std::endl;
                        return false;
                    }
                    
                    // Check if this is a separate sampler/image pair
                    // Separate samplers use DescriptorType::Sampler
                    // Separate images use SampledImage
                    // They can coexist on the same binding (though typically they're at different bindings)
                    if ((existingType == RHI::DescriptorType::SampledImage && tb.descriptorType == RHI::DescriptorType::Sampler) ||
                        (existingType == RHI::DescriptorType::Sampler && tb.descriptorType == RHI::DescriptorType::SampledImage)) {
                        // This is a separate sampler/image pair - they can coexist
                        // Don't update the binding map, keep the existing type
                        std::cerr << "    Note: Separate sampler/image pair detected at Set " << tb.set 
                                  << ", Binding " << tb.binding << " - allowing coexistence" << std::endl;
                    } else if (existingType == tb.descriptorType) {
                        // Same type on same binding - this might be an error or intentional
                        std::cerr << "Warning: MaterialDescriptorManager::CreateDescriptorSetLayouts: "
                                  << "Duplicate binding detected! Set " << tb.set << ", Binding " << tb.binding
                                  << " already has type " << static_cast<int>(existingType) 
                                  << ", trying to add same type (" << tb.name << ")" << std::endl;
                        // For now, we'll allow this and use the first one
                        // The descriptor set layout will only have one binding entry
                    } else {
                        // Different texture types on same binding - this is an error
                        std::cerr << "Error: MaterialDescriptorManager::CreateDescriptorSetLayouts: "
                                  << "Binding conflict detected! Set " << tb.set << ", Binding " << tb.binding
                                  << " is already used by type " << static_cast<int>(bindings[tb.binding])
                                  << ", trying to add " << static_cast<int>(tb.descriptorType) << " (" << tb.name << ")" << std::endl;
                        return false;
                    }
                } else {
                    // No conflict, add to map
                    bindings[tb.binding] = tb.descriptorType;
                }

                // Only add binding if it doesn't already exist in the layout
                // This handles the case where separate sampler and separate image share the same binding
                // (though they shouldn't in Vulkan, but we handle it gracefully)
                bool bindingExists = false;
                for (const auto& existingBinding : setLayouts[tb.set].bindings) {
                    if (existingBinding.binding == tb.binding) {
                        bindingExists = true;
                        break;
                    }
                }
                
                if (!bindingExists) {
                    RHI::DescriptorBinding binding;
                    binding.binding = tb.binding;
                    binding.type = tb.descriptorType; // Use the descriptor type from shader reflection
                    binding.count = 1;
                    // Determine shader stages that use this texture (default to fragment stage)
                    binding.stageFlags = RHI::ShaderStage::Fragment;
                    setLayouts[tb.set].bindings.push_back(binding);
                } else {
                    std::cerr << "    Note: Skipping duplicate binding " << tb.binding 
                              << " for texture '" << tb.name << "' (already exists in layout)" << std::endl;
                }
                
            }

            // Create descriptor set layouts for each set
            for (const auto& [setIndex, layoutDesc] : setLayouts) {
                // Sort bindings by binding index to ensure correct order
                std::vector<RHI::DescriptorBinding> sortedBindings = layoutDesc.bindings;
                std::sort(sortedBindings.begin(), sortedBindings.end(), 
                    [](const RHI::DescriptorBinding& a, const RHI::DescriptorBinding& b) {
                        return a.binding < b.binding;
                    });
                
                // Create a new layout description with sorted bindings
                RHI::DescriptorSetLayoutDescription sortedLayoutDesc;
                sortedLayoutDesc.bindings = sortedBindings;
                
                // Debug: Print layout information for troubleshooting
                RHI::DescriptorSetLayoutHandle layout = device->CreateDescriptorSetLayout(sortedLayoutDesc);
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

            // Count textures by descriptor type
            const auto& textureBindings = material->GetTextureBindings();
            std::map<RHI::DescriptorType, uint32_t> textureTypeCounts;
            for (const auto& tb : textureBindings) {
                textureTypeCounts[tb.descriptorType]++;
            }
            // Add pool sizes for each descriptor type
            for (const auto& [type, count] : textureTypeCounts) {
                if (count > 0) {
                    poolSizes.push_back({type, count});
                }
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

        void MaterialDescriptorManager::WriteDescriptorSets(ShadingMaterial* material, RHI::IDevice* device, bool updateUniformBuffers) {
            if (!material || !device) {
                return;
            }

            // IMPORTANT: This function updates descriptor sets, which may be in use by pending command buffers.
            // Even with UPDATE_UNUSED_WHILE_PENDING_BIT, we can only update descriptors that are NOT actually used by shaders.
            // If a descriptor is used (e.g., binding 0 uniform buffer), it cannot be updated while in use.
            // 
            // This function should be called:
            // 1. Before command buffer recording (in FlushParametersToGPU, which is called in EntityToRenderItems)
            // 2. Or after waiting for previous command buffers to complete
            //
            // For per-frame data (like PerFrame uniform buffer), consider using multiple descriptor set instances
            // (one per frame) to avoid this issue.

            std::vector<RHI::DescriptorWrite> writes;

            // Write uniform buffers to descriptor sets
            // NOTE: We only need to update descriptor set bindings if the buffer pointer changes.
            // If only the buffer content changes (via UpdateData), we don't need to update the descriptor set.
            // This avoids validation warnings when descriptor sets are in use by pending command buffers.
            // 
            // For uniform buffers, the buffer pointer typically doesn't change between frames,
            // only the buffer content changes. So we skip updating descriptor set bindings for uniform buffers
            // in UpdateBindings() (updateUniformBuffers=false) to avoid validation warnings. This is safe because:
            // 1. Buffer content is updated via UpdateData() in DoUpdate() (called from FlushParametersToGPU)
            // 2. Descriptor set binding (buffer pointer) only needs to be updated if the buffer object changes
            // 3. Initial binding is done in Initialize() (updateUniformBuffers=true), so bindings are already set up correctly
            //
            // However, we still update texture bindings because textures may change.
            if (updateUniformBuffers) {
                const auto& uniformBuffers = material->GetUniformBuffers();
                for (const auto& ub : uniformBuffers) {
                    if (!ub.buffer) {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Uniform buffer is nullptr for binding " 
                                  << ub.binding << " in set " << ub.set << std::endl;
                        continue;
                    }

                    RHI::DescriptorSetHandle set = GetDescriptorSet(ub.set);
                    if (!set) {
                        std::cerr << "Error: MaterialDescriptorManager::WriteDescriptorSets: Descriptor set " 
                                  << ub.set << " not found for uniform buffer binding " << ub.binding << std::endl;
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
            }

            // Write textures to descriptor sets
            // NOTE: We only update texture bindings if:
            // 1. Texture pointer has changed (checked via cache)
            // 2. Descriptor set hasn't been updated this frame (to avoid updating sets in use by command buffers)
            // This avoids validation warnings when descriptor sets are in use by pending command buffers.
            const auto& textureBindings = material->GetTextureBindings();
            for (const auto& tb : textureBindings) {
                RHI::DescriptorSetHandle set = GetDescriptorSet(tb.set);
                if (!set) {
                    std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Descriptor set " 
                              << tb.set << " not found for texture binding " << tb.binding << std::endl;
                    continue;
                }
                
                // Check if this descriptor set has already been updated this frame
                // If so, skip to avoid updating a descriptor set that's in use by a command buffer
                if (m_UpdatedThisFrame.find(set) != m_UpdatedThisFrame.end()) {
                    // Descriptor set already updated this frame - skip to avoid validation errors
                    // This is normal when multiple entities share the same material:
                    // - First entity updates the descriptor set
                    // - Subsequent entities skip the update (texture pointers haven't changed)
                    // - Each entity still updates its per-object uniform buffer data via UpdateData()
                    continue;
                }
                
                // Check if texture pointer has changed (only for non-null textures)
                auto cacheKey = std::make_pair(tb.set, tb.binding);
                auto it = m_LastTexturePointers.find(cacheKey);
                if (it != m_LastTexturePointers.end() && it->second == tb.texture && tb.texture != nullptr) {
                    // Texture pointer hasn't changed - skip update to avoid validation warnings
                    // But mark the set as "updated" so we don't try again this frame
                    m_UpdatedThisFrame.insert(set);
                    continue;
                }
                
                // Update cache
                m_LastTexturePointers[cacheKey] = tb.texture;

                RHI::DescriptorWrite write;
                write.dstSet = set;
                write.dstBinding = tb.binding;
                write.dstArrayElement = 0;
                write.descriptorType = tb.descriptorType; // Use the descriptor type from shader reflection

                // Handle different descriptor types
                if (tb.descriptorType == RHI::DescriptorType::CombinedImageSampler) {
                    // Combined image sampler needs both image and sampler
                    if (!tb.texture) {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Texture is nullptr for CombinedImageSampler binding " 
                                  << tb.binding << " in set " << tb.set << std::endl;
                        continue; // Skip if texture is missing
                    }
                    RHI::DescriptorImageInfo imageInfo;
                    imageInfo.image = tb.texture;
                    imageInfo.imageView = tb.texture->CreateImageView();
                    if (!imageInfo.imageView) {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Failed to create image view for texture binding " 
                                  << tb.binding << " in set " << tb.set << std::endl;
                        continue; // Skip this write if image view creation failed
                    }
                    // Get default sampler from device
                    Device::VulkanDevice* vkDevice = dynamic_cast<Device::VulkanDevice*>(device);
                    if (vkDevice) {
                        imageInfo.sampler = vkDevice->GetDefaultSampler();
                    } else {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Cannot get default sampler, device is not VulkanDevice" << std::endl;
                        imageInfo.sampler = nullptr;
                    }
                    write.imageInfo.push_back(imageInfo);
                } else if (tb.descriptorType == RHI::DescriptorType::Sampler) {
                    // Separate sampler: only needs sampler, no image or imageView
                    RHI::DescriptorImageInfo samplerInfo;
                    samplerInfo.image = nullptr;        // Separate sampler doesn't need image
                    samplerInfo.imageView = nullptr;   // Separate sampler doesn't need imageView
                    // Get default sampler from device
                    Device::VulkanDevice* vkDevice = dynamic_cast<Device::VulkanDevice*>(device);
                    if (vkDevice) {
                        samplerInfo.sampler = vkDevice->GetDefaultSampler();
                        if (!samplerInfo.sampler) {
                            std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Cannot get default sampler for separate sampler binding " 
                                      << tb.binding << " in set " << tb.set << std::endl;
                            continue; // Skip if we can't get sampler
                        }
                    } else {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Cannot get default sampler for separate sampler binding " 
                                  << tb.binding << " in set " << tb.set << ", device is not VulkanDevice" << std::endl;
                        continue; // Skip if we can't get sampler
                    }
                    write.imageInfo.push_back(samplerInfo);
                } else if (tb.descriptorType == RHI::DescriptorType::SampledImage) {
                    // Sampled image only needs image (sampler is separate)
                    // If texture is nullptr, skip this binding (material may not have this texture)
                    if (!tb.texture) {
                        // This is normal for optional textures - just skip this binding
                        // Don't write anything for this binding, it will remain uninitialized
                        // With descriptorBindingPartiallyBound enabled, this is safe
                        continue;
                    }
                    RHI::DescriptorImageInfo imageInfo;
                    imageInfo.image = tb.texture;
                    imageInfo.imageView = tb.texture->CreateImageView();
                    if (!imageInfo.imageView) {
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Failed to create image view for sampled image binding " 
                                  << tb.binding << " in set " << tb.set << std::endl;
                        continue;
                    }
                    imageInfo.sampler = nullptr; // Separate sampler, not combined
                    write.imageInfo.push_back(imageInfo);
                } else {
                    // Unknown descriptor type for texture binding
                    std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: Unsupported descriptor type " 
                              << static_cast<int>(tb.descriptorType) << " for texture binding " 
                              << tb.binding << " in set " << tb.set << std::endl;
                    continue;
                }

                writes.push_back(write);
            }

            // Update descriptor sets
            if (!writes.empty()) {
                device->UpdateDescriptorSets(writes);
                
                // Mark all updated descriptor sets as "updated this frame"
                // This prevents them from being updated again in the same frame
                for (const auto& write : writes) {
                    m_UpdatedThisFrame.insert(write.dstSet);
                }
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
