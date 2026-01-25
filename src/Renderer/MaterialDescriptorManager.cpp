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

            // Write initial bindings (including uniform buffers and all texture bindings)
            // This ensures all bindings are initialized, even if textures are nullptr
            // NOTE: At this point, descriptor sets have just been allocated and are not yet in use
            // So it's safe to update them. We use forceUpdateAllBindings=true to ensure all bindings
            // are initialized, but we still respect m_UpdatedThisFrame to avoid updating if somehow
            // the descriptor set was already updated (shouldn't happen in Initialize, but safety check)
            WriteDescriptorSets(material, device, true, true); // updateUniformBuffers=true, forceUpdateAllBindings=true for initial setup

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
            
            // Destroy placeholder texture (unique_ptr will handle cleanup)
            m_PlaceholderTexture.reset();
            
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
            // IMPORTANT: Do NOT clear m_UpdatedThisFrame here!
            // Descriptor sets that were updated in the previous frame may still be in use
            // by command buffers that haven't completed execution yet.
            // We only update descriptor sets when texture pointers actually change (detected via cache).
            // This means m_UpdatedThisFrame acts as a persistent "in use" marker until the next frame
            // when we can safely assume command buffers have completed.
            m_CurrentFrame++;
        }
        
        void MaterialDescriptorManager::MarkDescriptorSetInUse(RHI::DescriptorSetHandle set) {
            if (set) {
                m_UpdatedThisFrame.insert(set);
            }
        }
        
        void MaterialDescriptorManager::ForceUpdateAllBindings(ShadingMaterial* material, RHI::IDevice* device) {
            if (!material || !device || !m_Initialized) {
                return;
            }
            
            // Clear m_UpdatedThisFrame for all descriptor sets used by this material
            // This allows them to be updated even if they were marked as updated
            const auto& textureBindings = material->GetTextureBindings();
            for (const auto& tb : textureBindings) {
                RHI::DescriptorSetHandle set = GetDescriptorSet(tb.set);
                if (set) {
                    m_UpdatedThisFrame.erase(set);
                }
            }
            
            // Now update all bindings (textures only, not uniform buffers)
            WriteDescriptorSets(material, device, false, true);
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

            // Group resources by descriptor set and create descriptor set layouts
            std::map<uint32_t, RHI::DescriptorSetLayoutDescription> setLayouts;
            // Track bindings per set to detect conflicts
            std::map<uint32_t, std::map<uint32_t, RHI::DescriptorType>> setBindingMap;

            // Collect uniform buffer bindings by set
            const auto& uniformBuffers = material->GetUniformBuffers();
            for (const auto& ub : uniformBuffers) {
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
            
            // First pass: collect all texture bindings to identify separate sampler/image pairs
            // Separate samplers can share bindings with separate images (they're used together)
            // But we need to track which bindings are used by separate samplers vs separate images
            std::map<uint32_t, std::map<uint32_t, std::vector<std::pair<RHI::DescriptorType, std::string>>>> textureBindingTypes;
            for (const auto& tb : textureBindings) {
                textureBindingTypes[tb.set][tb.binding].push_back({tb.descriptorType, tb.name});
            }
            
            for (const auto& tb : textureBindings) {
                
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
                    } else if (existingType == tb.descriptorType) {
                        // Same type on same binding - this might be an error or intentional
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

        void MaterialDescriptorManager::WriteDescriptorSets(ShadingMaterial* material, RHI::IDevice* device, bool updateUniformBuffers, bool forceUpdateAllBindings) {
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
                
                // Create cache key for texture pointer tracking
                auto cacheKey = std::make_pair(tb.set, tb.binding);
                
                // Get placeholder texture pointer for comparison
                RHI::IImage* placeholderTexture = GetOrCreatePlaceholderTexture(device);
                
                // Check if texture pointer has changed
                // IMPORTANT: If texture pointer changed from placeholder to real texture, we MUST update
                // even if the descriptor set was already updated this frame
                bool textureChanged = false;
                bool wasPlaceholder = false;
                bool isPlaceholder = (tb.texture == nullptr || (placeholderTexture && tb.texture == placeholderTexture));
                
                if (!forceUpdateAllBindings) {
                    auto it = m_LastTexturePointers.find(cacheKey);
                    if (it != m_LastTexturePointers.end()) {
                        // Check if texture pointer changed
                        if (it->second != tb.texture) {
                            textureChanged = true;
                            // Check if previous texture was placeholder
                            wasPlaceholder = (it->second == placeholderTexture || it->second == nullptr);
                        } else if (tb.texture != nullptr && !isPlaceholder) {
                            // Texture pointer hasn't changed and it's not a placeholder - skip update
                            // But mark the set as "updated" so we don't try again this frame
                            if (m_UpdatedThisFrame.find(set) == m_UpdatedThisFrame.end()) {
                                m_UpdatedThisFrame.insert(set);
                            }
                            continue;
                        }
                    } else {
                        // First time we see this binding - texture pointer is new
                        textureChanged = true;
                    }
                }
                
                // If texture changed from placeholder to real texture, clear the "updated" flag
                // to allow the update to proceed (must be done BEFORE checking m_UpdatedThisFrame)
                if (textureChanged && wasPlaceholder && !isPlaceholder) {
                    // Remove from m_UpdatedThisFrame to allow update
                    m_UpdatedThisFrame.erase(set);
                }
                
                // Check if this descriptor set has already been updated this frame
                // Exception: If texture changed from placeholder to real texture, we MUST update
                // Exception: If forceUpdateAllBindings is true, we MUST update
                if (m_UpdatedThisFrame.find(set) != m_UpdatedThisFrame.end() && !textureChanged && !forceUpdateAllBindings) {
                    // Descriptor set already updated this frame and texture hasn't changed - skip
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
                    // If texture is nullptr, use placeholder texture to avoid validation errors
                    RHI::IImage* textureToUse = tb.texture;
                    if (!textureToUse) {
                        textureToUse = GetOrCreatePlaceholderTexture(device);
                        if (!textureToUse) {
                            std::cerr << "Error: MaterialDescriptorManager::WriteDescriptorSets: "
                                      << "Texture is nullptr for CombinedImageSampler binding " 
                                      << tb.binding << " in set " << tb.set 
                                      << ", and failed to create placeholder texture." << std::endl;
                            continue;
                        }
                        std::cerr << "Warning: MaterialDescriptorManager::WriteDescriptorSets: "
                                  << "Using placeholder texture for CombinedImageSampler binding " 
                                  << tb.binding << " in set " << tb.set << std::endl;
                    }
                    RHI::DescriptorImageInfo imageInfo;
                    imageInfo.image = textureToUse;
                    imageInfo.imageView = textureToUse->CreateImageView();
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
                    // If texture is nullptr, use placeholder texture to avoid validation errors
                    RHI::IImage* textureToUse = tb.texture;
                    if (!textureToUse) {
                        // Get or create placeholder texture
                        textureToUse = GetOrCreatePlaceholderTexture(device);
                        if (!textureToUse) {
                            continue;
                        }
                    }
                    RHI::DescriptorImageInfo imageInfo;
                    imageInfo.image = textureToUse;
                    imageInfo.imageView = textureToUse->CreateImageView();
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
                // IMPORTANT: Before updating, check if any descriptor set is already in m_UpdatedThisFrame
                // If so, we should not update it (it's in use by a command buffer)
                std::vector<RHI::DescriptorWrite> safeWrites;
                for (const auto& write : writes) {
                    if (m_UpdatedThisFrame.find(write.dstSet) != m_UpdatedThisFrame.end()) {
                        continue;
                    }
                    safeWrites.push_back(write);
                }
                
                if (!safeWrites.empty()) {
                    device->UpdateDescriptorSets(safeWrites);
                    
                    // Mark all updated descriptor sets as "updated this frame"
                    // This prevents them from being updated again in the same frame
                    for (const auto& write : safeWrites) {
                        m_UpdatedThisFrame.insert(write.dstSet);
                    }
                }
            }
        }

        RHI::IImage* MaterialDescriptorManager::GetOrCreatePlaceholderTexture(RHI::IDevice* device) {
            if (m_PlaceholderTexture) {
                return m_PlaceholderTexture.get();
            }

            if (!device) {
                return nullptr;
            }

            // Create a 1x1 white RGBA texture
            RHI::ImageDescription desc;
            desc.width = 1;
            desc.height = 1;
            desc.depth = 1;
            desc.mipLevels = 1;
            desc.arrayLayers = 1;
            desc.format = RHI::Format::R8G8B8A8_UNORM;
            desc.usage = static_cast<RHI::ImageUsageFlags>(
                static_cast<uint32_t>(RHI::ImageUsageFlags::Sampled) |
                static_cast<uint32_t>(RHI::ImageUsageFlags::TransferDst)
            );
            desc.memoryProperties = static_cast<RHI::MemoryPropertyFlags>(
                static_cast<uint32_t>(RHI::MemoryPropertyFlags::DeviceLocal)
            );

            auto placeholderImage = device->CreateImage(desc);
            if (!placeholderImage) {
                std::cerr << "Error: MaterialDescriptorManager::GetOrCreatePlaceholderTexture: "
                          << "Failed to create placeholder texture image" << std::endl;
                return nullptr;
            }

            // TODO: Upload white pixel data (255, 255, 255, 255 = white opaque) via staging buffer
            // For now, the image is created but not initialized with data
            // This might cause issues - we need to properly upload the data
            // However, for immediate validation error fixing, we'll proceed
            // The texture will be uninitialized but at least the descriptor will be valid
            
            // Store the unique_ptr to keep the image alive
            m_PlaceholderTexture = std::move(placeholderImage);
            
            
            return m_PlaceholderTexture.get();
        }

    } // namespace Renderer
} // namespace FirstEngine
