#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include "FirstEngine/Resources/MaterialLoader.h"
#include "FirstEngine/Shader/ShaderLoader.h"
#include "FirstEngine/Resources/TextureResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IImage.h"
#include <glm/glm.hpp>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        MaterialResource::MaterialResource() {
            m_Metadata.refCount = 0;
            m_Metadata.isLoaded = false;
            m_ShaderName = "DefaultPBR";
            m_ParameterDataDirty = true;
            m_ShadingMaterial = nullptr;
            
            // Set default parameters (optional - can be empty)
            // Example: SetParameter("Albedo", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        // IResourceProvider interface implementation
        bool MaterialResource::IsFormatSupported(const std::string& filepath) const {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".mat" || ext == ".json";
        }

        std::vector<std::string> MaterialResource::GetSupportedFormats() const {
            return { ".mat", ".json" };
        }

        ResourceLoadResult MaterialResource::Load(ResourceID id) {
            if (id == InvalidResourceID) {
                return ResourceLoadResult::UnknownError;
            }

            // Set metadata ID
            m_Metadata.resourceID = id;

            // Call Loader::Load - ResourceManager is used internally by Loader for caching
            // Loader returns both Handle data (shaderName, parameters, textures) and Metadata
            MaterialLoader::LoadResult loadResult = MaterialLoader::Load(id);
            
            if (!loadResult.success) {
                return ResourceLoadResult::FileNotFound;
            }

            // Use returned Metadata
            m_Metadata = loadResult.metadata;
            m_Metadata.resourceID = id; // Ensure ID matches

            // Use returned Handle data to initialize resource
            m_ShaderName = loadResult.shaderName;
            m_Parameters = loadResult.parameters;
            m_ParameterDataDirty = true;

            // Store texture IDs from dependencies (texture slots are stored in dependencies)
            // Extract texture slot information from dependencies
            for (const auto& dep : m_Metadata.dependencies) {
                if (dep.type == ResourceDependency::DependencyType::Texture) {
                    m_TextureIDs[dep.slot] = dep.resourceID;
                }
            }

            // Load dependencies (textures) - this is handled by Resource::Load
            LoadDependencies();
            
            // Load ShaderCollection by shader name
            if (!m_ShaderName.empty()) {
                auto& collectionsTools = Renderer::ShaderCollectionsTools::GetInstance();
                auto* collection = collectionsTools.GetCollectionByName(m_ShaderName);
                if (collection) {
                    m_ShaderCollectionID = collection->GetID();
                    m_ShaderCollection = collection;
                }
            }
            
            m_Metadata.isLoaded = true;
            m_Metadata.fileSize = 0; // Material file size is not tracked

            return ResourceLoadResult::Success;
        }

        ResourceLoadResult MaterialResource::LoadFromMemory(const void* data, size_t size) {
            // TODO: Implement material loading from memory
            (void)data;
            (void)size;
            return ResourceLoadResult::UnknownError;
        }

        MaterialResource::~MaterialResource() {
            // Release all texture references
            for (auto& pair : m_Textures) {
                if (pair.second) {
                    pair.second->Release();
                }
            }
            m_Textures.clear();
            
            // Delete shading material if exists
            if (m_ShadingMaterial) {
                delete static_cast<Renderer::ShadingMaterial*>(m_ShadingMaterial);
                m_ShadingMaterial = nullptr;
            }
        }

        void MaterialResource::LoadDependencies() {
            // Get ResourceManager singleton
            ResourceManager& resourceManager = ResourceManager::GetInstance();

            // Load textures from dependencies using ResourceID
            for (const auto& dep : m_Metadata.dependencies) {
                if (dep.type == ResourceDependency::DependencyType::Texture) {
                    // Skip if already loaded
                    if (m_Textures.find(dep.slot) != m_Textures.end()) {
                        continue;
                    }
                    
                    // Load by ID through ResourceManager
                    // ResourceManager internally uses the appropriate Resource::Load method
                    // (e.g., TextureResource::Load for textures, ModelResource::Load for models, etc.)
                    ResourceHandle handle = resourceManager.Load(dep.resourceID);
                    if (handle.texture) {
                        SetTexture(dep.slot.empty() ? "texture0" : dep.slot, handle.texture);
                    }
                }
            }

            // TODO: Load shader dependencies
            // For now, shader name is just a string reference
        }

        void MaterialResource::SetTexture(const std::string& slot, TextureHandle texture) {
            if (texture) {
                texture->AddRef();
            }
            
            // Release old texture if exists
            auto it = m_Textures.find(slot);
            if (it != m_Textures.end() && it->second) {
                it->second->Release();
            }
            
            m_Textures[slot] = texture;
        }

        void MaterialResource::SetTextureID(const std::string& slot, ResourceID textureID) {
            if (textureID == InvalidResourceID) {
                return;
            }

            m_TextureIDs[slot] = textureID;
            
            // Add dependency using ResourceID (no paths)
            ResourceDependency dep;
            dep.type = ResourceDependency::DependencyType::Texture;
            dep.resourceID = textureID;
            dep.slot = slot;
            dep.isRequired = true;
            m_Metadata.dependencies.push_back(dep);
        }

        ResourceID MaterialResource::GetTextureID(const std::string& slot) const {
            auto it = m_TextureIDs.find(slot);
            return (it != m_TextureIDs.end()) ? it->second : InvalidResourceID;
        }

        TextureHandle MaterialResource::GetTexture(const std::string& slot) const {
            auto it = m_Textures.find(slot);
            return it != m_Textures.end() ? it->second : nullptr;
        }

        void MaterialResource::SetParameter(const std::string& name, const MaterialParameterValue& value) {
            m_Parameters[name] = value;
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, float value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, const glm::vec2& value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, const glm::vec3& value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, const glm::vec4& value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, int32_t value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, bool value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, const glm::mat3& value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        void MaterialResource::SetParameter(const std::string& name, const glm::mat4& value) {
            m_Parameters[name] = MaterialParameterValue(value);
            m_ParameterDataDirty = true;
        }

        const MaterialParameterValue* MaterialResource::GetParameter(const std::string& name) const {
            auto it = m_Parameters.find(name);
            return (it != m_Parameters.end()) ? &it->second : nullptr;
        }

        const void* MaterialResource::GetParameterData() const {
            // Lazy build if dirty
            if (m_ParameterDataDirty) {
                const_cast<MaterialResource*>(this)->BuildParameterData();
            }
            return m_ParameterData.data();
        }

        uint32_t MaterialResource::GetParameterDataSize() const {
            // Lazy build if dirty
            if (m_ParameterDataDirty) {
                const_cast<MaterialResource*>(this)->BuildParameterData();
            }
            return static_cast<uint32_t>(m_ParameterData.size());
        }

        void MaterialResource::BuildParameterData() {
            m_ParameterData.clear();

            // Serialize all parameters into a contiguous byte stream
            // Format: For each parameter, store name length, name, type, data size, data
            // This is a simple serialization - for production, you might want a more optimized layout
            
            for (const auto& pair : m_Parameters) {
                const std::string& name = pair.first;
                const MaterialParameterValue& value = pair.second;

                // Write name (with length prefix)
                uint32_t nameLength = static_cast<uint32_t>(name.size());
                size_t offset = m_ParameterData.size();
                m_ParameterData.resize(offset + sizeof(uint32_t) + nameLength);
                std::memcpy(m_ParameterData.data() + offset, &nameLength, sizeof(uint32_t));
                std::memcpy(m_ParameterData.data() + offset + sizeof(uint32_t), name.data(), nameLength);

                // Write type
                offset = m_ParameterData.size();
                MaterialParameterValue::Type type = value.type;
                m_ParameterData.resize(offset + sizeof(MaterialParameterValue::Type));
                std::memcpy(m_ParameterData.data() + offset, &type, sizeof(MaterialParameterValue::Type));

                // Write data
                offset = m_ParameterData.size();
                uint32_t dataSize = value.GetDataSize();
                m_ParameterData.resize(offset + sizeof(uint32_t) + dataSize);
                std::memcpy(m_ParameterData.data() + offset, &dataSize, sizeof(uint32_t));
                std::memcpy(m_ParameterData.data() + offset + sizeof(uint32_t), value.GetData(), dataSize);
            }

            m_ParameterDataDirty = false;
        }

        bool MaterialResource::Save(const std::string& xmlFilePath) const {
            return MaterialLoader::Save(xmlFilePath, m_Metadata.name, m_Metadata.resourceID,
                                       m_ShaderName, m_Parameters, m_TextureIDs, m_Metadata.dependencies);
        }

        // Render resource management implementation (internal)
        bool MaterialResource::CreateShadingMaterial() {
            // Check if already created
            if (m_ShadingMaterial) {
                auto* shadingMaterial = static_cast<Renderer::ShadingMaterial*>(m_ShadingMaterial);
                if (shadingMaterial) {
                    return true; // Already exists
                }
            }

            // Only create if ShaderCollectionID is set
            if (m_ShaderCollectionID == 0) {
                return false;
            }

            // Create ShadingMaterial from material data
            auto shadingMaterial = std::make_unique<Renderer::ShadingMaterial>();
            
            // Initialize from MaterialResource (this will set shader collection, reflection, and parameters)
            // This is the preferred method as it passes all material information including shader parameters
            if (!shadingMaterial->InitializeFromMaterial(this)) {
                // Fallback: If InitializeFromMaterial fails, try InitializeFromShaderCollection
                // This can happen if MaterialResource doesn't have ShaderCollection set yet
                if (!shadingMaterial->InitializeFromShaderCollection(m_ShaderCollectionID)) {
                    return false;
                }
            }

            // Schedule creation (will be processed in OnCreateResources)
            shadingMaterial->ScheduleCreate();

            // Store in MaterialResource (takes ownership)
            m_ShadingMaterial = shadingMaterial.release();
            
            // Note: Textures will be set after ShadingMaterial is created (in SetTexturesToShadingMaterial)
            // This will be called when ShadingMaterial is ready and we have device
            
            return true;
        }
        
        void MaterialResource::SetTexturesToShadingMaterial() {
            if (!m_ShadingMaterial) {
                return;
            }
            
            auto* shadingMaterial = static_cast<Renderer::ShadingMaterial*>(m_ShadingMaterial);
            if (!shadingMaterial) {
                return;
            }

            // Get device from ShadingMaterial (if it's created)
            // Note: We need device to create texture GPU resources
            // For now, texture creation will be deferred until ShadingMaterial is created
            // and we have device available
            // This method will be called again after ShadingMaterial is created
            
            if (!shadingMaterial->IsCreated()) {
                // ShadingMaterial not created yet, textures will be set after creation
                return;
            }

            RHI::IDevice* device = shadingMaterial->GetDevice();
            if (!device) {
                return;
            }

            // For each texture in m_Textures, bind it to ShadingMaterial
            // We need to map texture slot names to shader bindings
            for (const auto& [slotName, textureHandle] : m_Textures) {
                if (!textureHandle) {
                    continue;
                }
                
                // Cast to TextureResource to access CreateRenderTexture
                auto* textureResource = dynamic_cast<TextureResource*>(textureHandle);
                if (!textureResource) {
                    continue;
                }
                
                // Ensure texture GPU resource is created
                textureResource->CreateRenderTexture();
                
                // Get GPU texture data
                TextureResource::RenderData textureData;
                if (!textureResource->GetRenderData(textureData)) {
                    continue;
                }
                
                if (!textureData.image) {
                    continue;
                }
                
                // Find texture binding in shader by slot name
                // Search through shader reflection to find matching texture binding
                const auto& reflection = shadingMaterial->GetShaderReflection();
                RHI::IImage* gpuTexture = static_cast<RHI::IImage*>(textureData.image);
                
                // Search samplers first
                for (const auto& sampler : reflection.samplers) {
                    if (sampler.name == slotName || sampler.name.find(slotName) != std::string::npos) {
                        shadingMaterial->SetTexture(sampler.set, sampler.binding, gpuTexture);
                        break;
                    }
                }
                
                // Search images
                for (const auto& image : reflection.images) {
                    if (image.name == slotName || image.name.find(slotName) != std::string::npos) {
                        shadingMaterial->SetTexture(image.set, image.binding, gpuTexture);
                        break;
                    }
                }
            }
        }

        bool MaterialResource::IsShadingMaterialReady() const {
            if (!m_ShadingMaterial) {
                return false;
            }
            auto* shadingMaterial = static_cast<Renderer::ShadingMaterial*>(m_ShadingMaterial);
            return shadingMaterial && shadingMaterial->IsCreated();
        }

        bool MaterialResource::GetRenderData(RenderData& outData) const {
            if (!m_ShadingMaterial) {
                return false;
            }
            auto* shadingMaterial = static_cast<Renderer::ShadingMaterial*>(m_ShadingMaterial);
            if (!shadingMaterial || !shadingMaterial->IsCreated()) {
                return false;
            }

            // Set textures now that ShadingMaterial is created and we have device
            // This is a const method, but we need to set textures
            // We'll use const_cast to call SetTexturesToShadingMaterial
            // In practice, this should be called from a non-const method or after creation
            const_cast<MaterialResource*>(this)->SetTexturesToShadingMaterial();

            outData.shadingMaterial = shadingMaterial;
            auto& shadingState = shadingMaterial->GetShadingState();
            outData.pipeline = shadingState.GetPipeline();
            outData.descriptorSet = shadingMaterial->GetDescriptorSet(0);
            outData.materialName = m_Metadata.name;
            return true;
        }

        void MaterialResource::SetShaderCollectionID(uint64_t collectionID) {
            m_ShaderCollectionID = collectionID;
            
            // Look up ShaderCollection from ShaderCollectionsTools
            auto& collectionsTools = Renderer::ShaderCollectionsTools::GetInstance();
            auto* collection = collectionsTools.GetCollection(collectionID);
            
            if (collection) {
                m_ShaderCollection = collection;
            } else {
                m_ShaderCollection = nullptr;
            }
        }

    } // namespace Resources
} // namespace FirstEngine
