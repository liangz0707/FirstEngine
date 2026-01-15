#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Resources/TextureResource.h"
#include "FirstEngine/Resources/MeshResource.h"
#include "FirstEngine/Resources/ModelResource.h"
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
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

        // Singleton instance
        std::unique_ptr<ResourceManager> ResourceManager::s_Instance = nullptr;

        // ResourceManager singleton implementation
        ResourceManager& ResourceManager::GetInstance() {
            if (!s_Instance) {
                Initialize();
            }
            return *s_Instance;
        }

        void ResourceManager::Initialize() {
            if (!s_Instance) {
                s_Instance = std::unique_ptr<ResourceManager>(new ResourceManager());
            }
        }

        void ResourceManager::Shutdown() {
            if (s_Instance) {
                s_Instance->Clear();
                s_Instance.reset();
            }
        }

        // ResourceManager implementation
        ResourceManager::ResourceManager() {
            // Register Resource classes as providers (for creating resource instances)
            RegisterProvider(ResourceType::Texture, std::make_unique<TextureResource>());
            RegisterProvider(ResourceType::Mesh, std::make_unique<MeshResource>());
            RegisterProvider(ResourceType::Material, std::make_unique<MaterialResource>());
            RegisterProvider(ResourceType::Model, std::make_unique<ModelResource>());
            
            // Add default search paths
            m_SearchPaths.push_back(".");
            m_SearchPaths.push_back("assets");
            m_SearchPaths.push_back("resources");
        }

        ResourceManager::~ResourceManager() {
            Clear();
        }

        void ResourceManager::RegisterProvider(ResourceType type, std::unique_ptr<IResourceProvider> provider) {
            if (provider) {
                m_Providers[type] = std::move(provider);
            }
        }

        IResourceProvider* ResourceManager::GetProvider(ResourceType type) const {
            auto it = m_Providers.find(type);
            return (it != m_Providers.end()) ? it->second.get() : nullptr;
        }

        ResourceType ResourceManager::DetectResourceType(const std::string& filepath) const {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // Texture formats
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || 
                ext == ".tga" || ext == ".dds" || ext == ".tiff" || ext == ".hdr" ||
                ext == ".exr" || ext == ".ktx") {
                return ResourceType::Texture;
            }

            // Material formats
            if (ext == ".mat" || ext == ".material" || ext == ".json") {
                return ResourceType::Material;
            }

            // Model formats
            if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb" ||
                ext == ".dae" || ext == ".3ds" || ext == ".blend" || ext == ".x") {
                return ResourceType::Model;
            }

            // Mesh formats (if separate from models)
            if (ext == ".mesh" || ext == ".pmx") {
                return ResourceType::Mesh;
            }

            // Shader formats
            if (ext == ".vert" || ext == ".frag" || ext == ".geom" || ext == ".comp" ||
                ext == ".glsl" || ext == ".hlsl" || ext == ".spv") {
                return ResourceType::Shader;
            }

            return ResourceType::Unknown;
        }

        std::string ResourceManager::ResolveResourcePath(const std::string& path, const std::string& basePath) const {
            if (path.empty()) {
                return path;
            }

            // If path is absolute, return as-is
            if (ResourcePathResolver::IsAbsolutePath(path)) {
                return ResourcePathResolver::NormalizePath(path);
            }

            // Try resolving relative to base path
            if (!basePath.empty()) {
                std::string resolved = ResourcePathResolver::ResolvePath(path, basePath);
                // Check if resolved path exists
                if (fs::exists(resolved)) {
                    return resolved;
                }
            }

            // Try resolving from search paths
            for (const auto& searchPath : m_SearchPaths) {
                std::string resolved = ResourcePathResolver::ResolvePath(path, searchPath);
                if (fs::exists(resolved)) {
                    return resolved;
                }
            }

            // If not found, return normalized path (caller will handle error)
            return ResourcePathResolver::NormalizePath(path);
        }

        // Primary load method using ResourceID
        ResourceHandle ResourceManager::Load(ResourceID id) {
            if (id == InvalidResourceID) {
                return ResourceHandle();
            }

            // Check cache first - if resource already loaded, return it
            ResourceHandle cached = Get(id);
            if (cached.ptr) {
                // Increment reference count
                static_cast<IResource*>(cached.ptr)->AddRef();
                return cached;
            }

            // Load using internal method
            return LoadInternal(id);
        }

        std::string ResourceManager::GetResolvedPath(ResourceID id) const {
            std::string filepath = m_IDManager.GetPathFromID(id);
            if (filepath.empty()) {
                return "";
            }
            return ResolveResourcePath(filepath);
        }

        ResourceHandle ResourceManager::LoadInternal(ResourceID id) {
            // Get resource path from ID
            std::string filepath = m_IDManager.GetPathFromID(id);
            if (filepath.empty()) {
                return ResourceHandle();
            }

            ResourceType type = m_IDManager.GetTypeFromID(id);
            if (type == ResourceType::Unknown) {
                type = DetectResourceType(filepath);
            }

            // Get provider for this resource type
            IResourceProvider* provider = GetProvider(type);
            if (!provider) {
                return ResourceHandle();
            }

            // Create a new resource instance based on type
            std::unique_ptr<IResource> resource;
            
            switch (type) {
                case ResourceType::Texture:
                    resource = std::make_unique<TextureResource>();
                    break;
                case ResourceType::Mesh:
                    resource = std::make_unique<MeshResource>();
                    break;
                case ResourceType::Material:
                    resource = std::make_unique<MaterialResource>();
                    break;
                case ResourceType::Model:
                    resource = std::make_unique<ModelResource>();
                    break;
                default:
                    return ResourceHandle();
            }

            if (!resource) {
                return ResourceHandle();
            }

            // Cast resource to IResourceProvider and call Load by ID
            IResourceProvider* resourceProvider = dynamic_cast<IResourceProvider*>(resource.get());
            if (!resourceProvider) {
                return ResourceHandle();
            }

            // Set resource ID in metadata before Load (so resource can access its ID)
            resource->GetMetadata().resourceID = id;

            // Resource object calls Load on itself with ID
            // Load flow: 1) Collect dependencies 2) Load dependencies 3) Load resource data 4) Initialize
            // ResourceManager is accessed via singleton, no parameter needed
            ResourceLoadResult result = resourceProvider->Load(id);
            if (result != ResourceLoadResult::Success) {
                return ResourceHandle();
            }

            // Store in cache using ID as key
            IResource* resourcePtr = resource.get();
            resourcePtr->AddRef(); // Initial reference count
            
            switch (type) {
                case ResourceType::Texture: {
                    m_LoadedTextures[id] = std::unique_ptr<ITexture>(static_cast<ITexture*>(resource.release()));
                    return ResourceHandle(static_cast<TextureHandle>(resourcePtr));
                }
                case ResourceType::Mesh: {
                    m_LoadedMeshes[id] = std::unique_ptr<IMesh>(static_cast<IMesh*>(resource.release()));
                    return ResourceHandle(static_cast<MeshHandle>(resourcePtr));
                }
                case ResourceType::Material: {
                    m_LoadedMaterials[id] = std::unique_ptr<IMaterial>(static_cast<IMaterial*>(resource.release()));
                    return ResourceHandle(static_cast<MaterialHandle>(resourcePtr));
                }
                case ResourceType::Model: {
                    m_LoadedModels[id] = std::unique_ptr<IModel>(static_cast<IModel*>(resource.release()));
                    return ResourceHandle(static_cast<ModelHandle>(resourcePtr));
                }
                default:
                    return ResourceHandle();
            }
        }

        // Legacy path-based load methods (for compatibility - auto-registers path and gets ID)
        ResourceHandle ResourceManager::Load(const std::string& filepath, const std::string& basePath) {
            ResourceType type = DetectResourceType(filepath);
            if (type == ResourceType::Unknown) {
                return ResourceHandle();
            }
            return Load(type, filepath, basePath);
        }

        ResourceHandle ResourceManager::Load(ResourceType type, const std::string& filepath, const std::string& basePath) {
            std::string resolvedPath = ResolveResourcePath(filepath, basePath);
            
            // Register path and get/create ID
            ResourceID id = m_IDManager.RegisterResource(resolvedPath, type);
            if (id == InvalidResourceID) {
                return ResourceHandle();
            }

            // Cache path to ID mapping
            m_PathToIDCache[resolvedPath] = id;

            // Load using ID
            return Load(id);
        }

        // Get resource by ID
        ResourceHandle ResourceManager::Get(ResourceID id) const {
            if (id == InvalidResourceID) {
                return ResourceHandle();
            }

            // Check all resource caches
            // Mesh
            {
                auto it = m_LoadedMeshes.find(id);
                if (it != m_LoadedMeshes.end()) {
                    return ResourceHandle(it->second.get());
                }
            }
            // Material
            {
                auto it = m_LoadedMaterials.find(id);
                if (it != m_LoadedMaterials.end()) {
                    return ResourceHandle(it->second.get());
                }
            }
            // Texture
            {
                auto it = m_LoadedTextures.find(id);
                if (it != m_LoadedTextures.end()) {
                    return ResourceHandle(it->second.get());
                }
            }
            // Model
            {
                auto it = m_LoadedModels.find(id);
                if (it != m_LoadedModels.end()) {
                    return ResourceHandle(it->second.get());
                }
            }

            return ResourceHandle();
        }

        // Legacy path-based get methods
        ResourceHandle ResourceManager::Get(const std::string& filepath) const {
            ResourceType type = DetectResourceType(filepath);
            if (type == ResourceType::Unknown) {
                return ResourceHandle();
            }
            return Get(type, filepath);
        }

        ResourceHandle ResourceManager::Get(ResourceType type, const std::string& filepath) const {
            std::string resolvedPath = ResolveResourcePath(filepath, "");
            
            // Try to get ID from cache first
            auto cacheIt = m_PathToIDCache.find(resolvedPath);
            if (cacheIt != m_PathToIDCache.end()) {
                return Get(cacheIt->second);
            }

            // Try to get ID from IDManager
            ResourceID id = m_IDManager.GetIDFromPath(resolvedPath);
            if (id != InvalidResourceID) {
                return Get(id);
            }

            return ResourceHandle();
        }

        // Unload by ID
        void ResourceManager::Unload(ResourceID id) {
            if (id == InvalidResourceID) {
                return;
            }

            ResourceType type = m_IDManager.GetTypeFromID(id);
            
            switch (type) {
                case ResourceType::Mesh: {
                    auto it = m_LoadedMeshes.find(id);
                    if (it != m_LoadedMeshes.end()) {
                        MeshHandle mesh = it->second.get();
                        mesh->Release();
                        if (mesh->GetRefCount() == 0) {
                            m_LoadedMeshes.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Material: {
                    auto it = m_LoadedMaterials.find(id);
                    if (it != m_LoadedMaterials.end()) {
                        MaterialHandle material = it->second.get();
                        material->Release();
                        if (material->GetRefCount() == 0) {
                            m_LoadedMaterials.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Texture: {
                    auto it = m_LoadedTextures.find(id);
                    if (it != m_LoadedTextures.end()) {
                        TextureHandle texture = it->second.get();
                        texture->Release();
                        if (texture->GetRefCount() == 0) {
                            m_LoadedTextures.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Model: {
                    auto it = m_LoadedModels.find(id);
                    if (it != m_LoadedModels.end()) {
                        ModelHandle model = it->second.get();
                        model->Release();
                        if (model->GetRefCount() == 0) {
                            m_LoadedModels.erase(it);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        void ResourceManager::Unload(ResourceHandle handle) {
            if (!handle.ptr) return;

            // Get ID from resource metadata
            IResource* resource = static_cast<IResource*>(handle.ptr);
            ResourceID id = resource->GetMetadata().resourceID;
            if (id != InvalidResourceID) {
                Unload(id);
            }
        }

        // Legacy path-based unload methods
        void ResourceManager::Unload(const std::string& filepath) {
            ResourceType type = DetectResourceType(filepath);
            if (type != ResourceType::Unknown) {
                Unload(type, filepath);
            }
        }

        void ResourceManager::Unload(ResourceType type, const std::string& filepath) {
            std::string resolvedPath = ResolveResourcePath(filepath, "");
            
            // Get ID from cache or IDManager
            ResourceID id = InvalidResourceID;
            auto cacheIt = m_PathToIDCache.find(resolvedPath);
            if (cacheIt != m_PathToIDCache.end()) {
                id = cacheIt->second;
            } else {
                id = m_IDManager.GetIDFromPath(resolvedPath);
            }

            if (id != InvalidResourceID) {
                Unload(id);
            }
        }

        void ResourceManager::AddSearchPath(const std::string& path) {
            std::string normalized = ResourcePathResolver::NormalizePath(path);
            if (std::find(m_SearchPaths.begin(), m_SearchPaths.end(), normalized) == m_SearchPaths.end()) {
                m_SearchPaths.push_back(normalized);
            }
        }

        void ResourceManager::RemoveSearchPath(const std::string& path) {
            std::string normalized = ResourcePathResolver::NormalizePath(path);
            m_SearchPaths.erase(
                std::remove(m_SearchPaths.begin(), m_SearchPaths.end(), normalized),
                m_SearchPaths.end()
            );
        }

        void ResourceManager::Clear() {
            m_LoadedMeshes.clear();
            m_LoadedMaterials.clear();
            m_LoadedTextures.clear();
            m_LoadedModels.clear();
            m_PathToIDCache.clear();
        }

    } // namespace Resources
} // namespace FirstEngine
