#include "FirstEngine/Resources/ModelResource.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/MeshResource.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Resources/ModelLoader.h"
#include "FirstEngine/Resources/MeshLoader.h"  // For Vertex definition
#include <memory>
#include <stdexcept>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#include <iostream>

namespace FirstEngine {
    namespace Resources {

        ModelResource::ModelResource() {
            m_Metadata.refCount = 0;
            m_Metadata.isLoaded = false;
        }

        ModelResource::~ModelResource() {
            // Release all mesh and material references
            for (auto* mesh : m_Meshes) {
                if (mesh) mesh->Release();
            }
            for (auto* material : m_Materials) {
                if (material) material->Release();
            }
            // Release all texture references
            for (auto& pair : m_Textures) {
                if (pair.second) {
                    pair.second->Release();
                }
            }
            m_Textures.clear();
        }

        // IResourceProvider interface implementation
        bool ModelResource::IsFormatSupported(const std::string& filepath) const {
            return ModelLoader::IsFormatSupported(filepath);
        }

        std::vector<std::string> ModelResource::GetSupportedFormats() const {
            return ModelLoader::GetSupportedFormats();
        }

        ResourceLoadResult ModelResource::Load(ResourceID id) {
            if (id == InvalidResourceID) {
                return ResourceLoadResult::UnknownError;
            }

            // Set metadata ID
            m_Metadata.resourceID = id;

            // Call Loader::Load - ResourceManager is used internally by Loader for caching
            // Loader returns Handle data (ResourceID references) and Metadata
            // Model is logical - it only contains ResourceID references to Mesh and Material resources
            ModelLoader::LoadResult loadResult = ModelLoader::Load(id);
            
            if (!loadResult.success) {
                return ResourceLoadResult::FileNotFound;
            }

            // Use returned Metadata
            m_Metadata = loadResult.metadata;
            m_Metadata.resourceID = id; // Ensure ID matches

            // Clear existing data
            m_Meshes.clear();
            m_Materials.clear();
            m_MeshNames.clear();

            // Load dependencies using unified LoadDependencies() method
            // This ensures consistent dependency loading logic across all Resource types
            LoadDependencies();

            // Verify that at least one mesh was loaded
            if (m_Meshes.empty()) {
                std::cerr << "ModelResource::Load: Model ID " << id << " has no meshes after loading dependencies" << std::endl;
                std::cerr << "  Total dependencies: " << m_Metadata.dependencies.size() << std::endl;
                size_t meshDepCount = 0;
                for (const auto& dep : m_Metadata.dependencies) {
                    if (dep.type == ResourceDependency::DependencyType::Mesh) {
                        meshDepCount++;
                    }
                }
                std::cerr << "  Mesh dependencies: " << meshDepCount << std::endl;
                return ResourceLoadResult::InvalidFormat;
            }

            m_Metadata.isLoaded = true;
            m_Metadata.fileSize = 0; // Model file size is not tracked (model is logical)

            return ResourceLoadResult::Success;
        }

        void ModelResource::LoadDependencies() {
            // Get ResourceManager singleton for loading dependencies
            ResourceManager& resourceManager = ResourceManager::GetInstance();

            // Load and connect dependencies based on type
            // Use ResourceManager::Load which internally calls the appropriate Resource::Load method
            // (e.g., ModelResource::Load for models, MeshResource::Load for meshes, etc.)
            // ResourceManager's cache mechanism prevents duplicate loading
            size_t meshCount = 0;
            size_t materialCount = 0;
            
            for (const auto& dep : m_Metadata.dependencies) {
                if (dep.resourceID == InvalidResourceID) {
                    std::cerr << "ModelResource::LoadDependencies: Skipping invalid dependency (ID=0) for model ID " << m_Metadata.resourceID << std::endl;
                    continue; // Skip invalid dependencies
                }

                // ResourceManager::Load internally uses ModelResource::Load, MeshResource::Load, etc.
                // If the dependency is already loaded, it will be returned from cache (with ref count incremented)
                ResourceHandle depHandle;
                try {
                    depHandle = resourceManager.Load(dep.resourceID);
                } catch (const std::exception& e) {
                    std::cerr << "ModelResource::LoadDependencies: Exception loading dependency ID " << dep.resourceID << " for model ID " << m_Metadata.resourceID << ": " << e.what() << std::endl;
                    continue;
                } catch (...) {
                    std::cerr << "ModelResource::LoadDependencies: Unknown exception loading dependency ID " << dep.resourceID << " for model ID " << m_Metadata.resourceID << std::endl;
                    continue;
                }
                
                if (!depHandle.ptr) {
                    std::cerr << "ModelResource::LoadDependencies: Failed to load dependency ID " << dep.resourceID << " (type=" << static_cast<int>(dep.type) << ") for model ID " << m_Metadata.resourceID << std::endl;
                    continue; // Skip failed dependencies
                }
                
                switch (dep.type) {
                    case ResourceDependency::DependencyType::Mesh: {
                        if (!depHandle.mesh) {
                            std::cerr << "ModelResource::LoadDependencies: Dependency ID " << dep.resourceID << " is not a mesh for model ID " << m_Metadata.resourceID << std::endl;
                            break;
                        }
                        
                        // Slot contains mesh index - if empty, use current mesh count
                        uint32_t meshIndex = 0;
                        if (!dep.slot.empty()) {
                            try {
                                meshIndex = static_cast<uint32_t>(std::stoul(dep.slot));
                            } catch (...) {
                                std::cerr << "ModelResource::LoadDependencies: Invalid mesh slot index: " << dep.slot << ", using index " << meshCount << std::endl;
                                meshIndex = static_cast<uint32_t>(meshCount);
                            }
                        } else {
                            meshIndex = static_cast<uint32_t>(meshCount);
                        }
                        
                        // Ensure we have enough slots
                        while (m_Meshes.size() <= meshIndex) {
                            m_Meshes.push_back(nullptr);
                            m_Materials.push_back(nullptr);
                            m_MeshNames.push_back("");
                        }
                        
                        // AddRef is already called by ResourceManager::Load
                        // But we need to add another ref since we're storing it
                        depHandle.mesh->AddRef();
                        m_Meshes[meshIndex] = depHandle.mesh;
                        meshCount++;
                        break;
                    }
                    case ResourceDependency::DependencyType::Material: {
                        if (depHandle.material && !dep.slot.empty()) {
                            // Slot contains material index
                            try {
                                uint32_t materialIndex = static_cast<uint32_t>(std::stoul(dep.slot));
                                
                                // Ensure we have enough slots
                                while (m_Materials.size() <= materialIndex) {
                                    m_Meshes.push_back(nullptr);
                                    m_Materials.push_back(nullptr);
                                    m_MeshNames.push_back("");
                                }
                                
                                SetMaterial(materialIndex, depHandle.material);
                            } catch (...) {
                                std::cerr << "ModelResource::LoadDependencies: Invalid material slot index: " << dep.slot << std::endl;
                            }
                        }
                        break;
                    }
                    case ResourceDependency::DependencyType::Texture: {
                        if (depHandle.texture) {
                            SetTexture(dep.slot.empty() ? "texture0" : dep.slot, depHandle.texture);
                        }
                        break;
                    }
                    default:
                        // Unknown dependency type, skip
                        break;
                }
            }
        }

        void ModelResource::ParseDependencies(const Model& model, const std::string& resolvedPath) {
            // Clear existing dependencies
            m_Metadata.dependencies.clear();

            // Get ResourceManager singleton
            ResourceManager& resourceManager = ResourceManager::GetInstance();

            // Get base path from current resource's file path (internal use only - for resolving relative paths)
            // Path handling is completely hidden - this is an internal implementation detail
            std::string basePath = ResourcePathResolver::GetDirectory(resolvedPath);

            // Extract material dependencies from model meshes
            for (size_t i = 0; i < model.meshes.size(); ++i) {
                const auto& mesh = model.meshes[i];
                
                if (!mesh.materialName.empty()) {
                    // Resolve material path relative to model file's directory
                    // Path resolution is handled internally by ResourceManager singleton - not exposed to users
                    std::string materialPath = resourceManager.ResolveResourcePath(mesh.materialName, basePath);
                    
                    // Register material path and get ResourceID (or get existing ID if already registered)
                    // All path operations are hidden - only ResourceID is used for dependencies
                    ResourceID materialID = resourceManager.RegisterResource(materialPath, ResourceType::Material);
                    
                    if (materialID != InvalidResourceID) {
                        ResourceDependency dep;
                        dep.type = ResourceDependency::DependencyType::Material;
                        dep.resourceID = materialID; // Store ResourceID, not path
                        dep.slot = std::to_string(i); // Use mesh index as slot to identify which mesh this material belongs to
                        dep.isRequired = true;
                        m_Metadata.dependencies.push_back(dep);
                    }
                }
            }
        }

        bool ModelResource::Initialize(const Model& model) {
            if (model.meshes.empty()) {
                return false;
            }

            // Get ResourceManager singleton
            ResourceManager& resourceManager = ResourceManager::GetInstance();

            m_Meshes.clear();
            m_Materials.clear();
            m_MeshNames.clear();
            // Dependencies are already parsed in ParseDependencies, don't clear them

            // Convert each mesh in the model to MeshResource
            for (size_t i = 0; i < model.meshes.size(); ++i) {
                const auto& mesh = model.meshes[i];
                
                // Create mesh resource from vertex data
                auto meshResource = std::make_unique<MeshResource>();
                if (!meshResource->Initialize(mesh.vertices, mesh.indices, sizeof(Vertex))) {
                    continue; // Skip invalid meshes
                }

                // Generate a unique key for this mesh (internal use only)
                std::string meshKey = m_Metadata.filePath + "_mesh_" + std::to_string(i);
                
                // Register mesh with ResourceManager and get ID (internal IDs for meshes)
                ResourceID meshID = resourceManager.RegisterResource(meshKey, ResourceType::Mesh);
                meshResource->GetMetadata().resourceID = meshID;
                meshResource->GetMetadata().filePath = meshKey; // Internal
                meshResource->GetMetadata().name = meshKey;
                
                MeshHandle meshHandle = meshResource.get();
                meshHandle->AddRef();
                m_Meshes.push_back(meshHandle);
                
                // Store unique_ptr to keep mesh alive
                m_MeshStorage.push_back(std::move(meshResource));

                // Store material placeholder (will be set from loaded dependencies)
                m_Materials.push_back(nullptr);
                m_MeshNames.push_back(mesh.materialName);
            }

            // Materials will be connected in LoadDependencies after they are loaded
            return !m_Meshes.empty();
        }

        MeshHandle ModelResource::GetMesh(uint32_t index) const {
            if (index >= m_Meshes.size()) return nullptr;
            return m_Meshes[index];
        }

        MaterialHandle ModelResource::GetMaterial(uint32_t index) const {
            if (index >= m_Materials.size()) return nullptr;
            return m_Materials[index];
        }

        void ModelResource::SetMaterial(uint32_t index, MaterialHandle material) {
            if (index >= m_Materials.size()) {
                return;
            }
            
            // Release old material if exists
            if (m_Materials[index]) {
                m_Materials[index]->Release();
            }
            
            m_Materials[index] = material;
            if (material) {
                material->AddRef();
            }
        }

        const std::string& ModelResource::GetMeshName(uint32_t index) const {
            static const std::string empty;
            if (index >= m_MeshNames.size()) return empty;
            return m_MeshNames[index];
        }

        TextureHandle ModelResource::GetTexture(const std::string& slot) const {
            auto it = m_Textures.find(slot);
            return it != m_Textures.end() ? it->second : nullptr;
        }

        void ModelResource::SetTexture(const std::string& slot, TextureHandle texture) {
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

        bool ModelResource::Save(const std::string& xmlFilePath) const {
            // Collect mesh indices
            std::vector<std::pair<uint32_t, ResourceID>> meshIndices;
            for (size_t i = 0; i < m_Meshes.size(); ++i) {
                if (m_Meshes[i]) {
                    ResourceID meshID = m_Meshes[i]->GetMetadata().resourceID;
                    if (meshID != InvalidResourceID) {
                        meshIndices.push_back({static_cast<uint32_t>(i), meshID});
                    }
                }
            }

            // Collect material indices
            std::vector<std::pair<uint32_t, ResourceID>> materialIndices;
            for (size_t i = 0; i < m_Materials.size(); ++i) {
                if (m_Materials[i]) {
                    ResourceID materialID = m_Materials[i]->GetMetadata().resourceID;
                    if (materialID != InvalidResourceID) {
                        materialIndices.push_back({static_cast<uint32_t>(i), materialID});
                    }
                }
            }

            // Collect texture slots
            std::vector<std::pair<std::string, ResourceID>> textureSlots;
            for (const auto& pair : m_Textures) {
                if (pair.second) {
                    ResourceID textureID = pair.second->GetMetadata().resourceID;
                    if (textureID != InvalidResourceID) {
                        textureSlots.push_back({pair.first, textureID});
                    }
                }
            }

            return ModelLoader::Save(xmlFilePath, m_Metadata.name, m_Metadata.resourceID,
                                    meshIndices, materialIndices, textureSlots, m_Metadata.dependencies);
        }

    } // namespace Resources
} // namespace FirstEngine
