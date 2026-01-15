#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include "FirstEngine/Resources/MaterialLoader.h"
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

    } // namespace Resources
} // namespace FirstEngine
