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
            // Add default search paths
            m_SearchPaths.push_back(".");
            m_SearchPaths.push_back("assets");
            m_SearchPaths.push_back("resources");
        }

        ResourceManager::~ResourceManager() {
            Clear();
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
        // This method internally creates the appropriate Resource instance (ModelResource, MeshResource, etc.)
        // and calls the Resource's Load method (e.g., ModelResource::Load) for unified loading interface
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

            // Create a new resource instance based on type and directly call its Load method
            // All Resource classes (TextureResource, MeshResource, MaterialResource, ModelResource) 
            // implement both IResource and IResourceProvider interfaces
            IResource* resource = nullptr;
            
            switch (type) {
                case ResourceType::Texture:
                    resource = new TextureResource();
                    break;
                case ResourceType::Mesh:
                    resource = new MeshResource();
                    break;
                case ResourceType::Material:
                    resource = new MaterialResource();
                    break;
                case ResourceType::Model:
                    resource = new ModelResource();
                    break;
                default:
                    return ResourceHandle();
            }

            if (!resource) {
                return ResourceHandle();
            }

            // Get IResourceProvider interface from resource (all Resource classes implement IResourceProvider)
            IResourceProvider* resourceProvider = dynamic_cast<IResourceProvider*>(resource);
            if (!resourceProvider) {
                delete resource;
                return ResourceHandle();
            }

            // Set resource ID in metadata before Load (so resource can access its ID)
            resource->GetMetadata().resourceID = id;

            // Directly call Resource's Load method (e.g., ModelResource::Load, MeshResource::Load)
            // This ensures unified loading interface without using provider mechanism
            // Load flow: 1) Collect dependencies 2) Load dependencies 3) Load resource data 4) Initialize
            // ResourceManager is accessed via singleton, no parameter needed
            ResourceLoadResult result = resourceProvider->Load(id);
            if (result != ResourceLoadResult::Success) {
                delete resource;
                return ResourceHandle();
            }

            // Store in cache using ID as key
            resource->AddRef(); // Initial reference count
            
            switch (type) {
                case ResourceType::Texture: {
                    ITexture* texture = static_cast<ITexture*>(resource);
                    m_LoadedTextures[id] = texture;
                    return ResourceHandle(static_cast<TextureHandle>(texture));
                }
                case ResourceType::Mesh: {
                    IMesh* mesh = static_cast<IMesh*>(resource);
                    m_LoadedMeshes[id] = mesh;
                    return ResourceHandle(static_cast<MeshHandle>(mesh));
                }
                case ResourceType::Material: {
                    IMaterial* material = static_cast<IMaterial*>(resource);
                    m_LoadedMaterials[id] = material;
                    return ResourceHandle(static_cast<MaterialHandle>(material));
                }
                case ResourceType::Model: {
                    IModel* model = static_cast<IModel*>(resource);
                    m_LoadedModels[id] = model;
                    return ResourceHandle(static_cast<ModelHandle>(model));
                }
                default:
                    delete resource;
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
                    return ResourceHandle(it->second);
                }
            }
            // Material
            {
                auto it = m_LoadedMaterials.find(id);
                if (it != m_LoadedMaterials.end()) {
                    return ResourceHandle(it->second);
                }
            }
            // Texture
            {
                auto it = m_LoadedTextures.find(id);
                if (it != m_LoadedTextures.end()) {
                    return ResourceHandle(it->second);
                }
            }
            // Model
            {
                auto it = m_LoadedModels.find(id);
                if (it != m_LoadedModels.end()) {
                    return ResourceHandle(it->second);
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
                        IMesh* mesh = it->second;
                        mesh->Release();
                        if (mesh->GetRefCount() == 0) {
                            delete mesh;
                            m_LoadedMeshes.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Material: {
                    auto it = m_LoadedMaterials.find(id);
                    if (it != m_LoadedMaterials.end()) {
                        IMaterial* material = it->second;
                        material->Release();
                        if (material->GetRefCount() == 0) {
                            delete material;
                            m_LoadedMaterials.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Texture: {
                    auto it = m_LoadedTextures.find(id);
                    if (it != m_LoadedTextures.end()) {
                        ITexture* texture = it->second;
                        texture->Release();
                        if (texture->GetRefCount() == 0) {
                            delete texture;
                            m_LoadedTextures.erase(it);
                        }
                    }
                    break;
                }
                case ResourceType::Model: {
                    auto it = m_LoadedModels.find(id);
                    if (it != m_LoadedModels.end()) {
                        IModel* model = it->second;
                        model->Release();
                        if (model->GetRefCount() == 0) {
                            delete model;
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

        // Resource ID management methods (encapsulate ResourceIDManager)
        ResourceID ResourceManager::RegisterResource(const std::string& filepath, ResourceType type, const std::string& virtualPath) {
            return m_IDManager.RegisterResource(filepath, type, virtualPath);
        }

        ResourceID ResourceManager::GetIDFromPath(const std::string& filepath) const {
            return m_IDManager.GetIDFromPath(filepath);
        }

        std::string ResourceManager::GetPathFromID(ResourceID id) const {
            return m_IDManager.GetPathFromID(id);
        }

        ResourceType ResourceManager::GetTypeFromID(ResourceID id) const {
            return m_IDManager.GetTypeFromID(id);
        }

        bool ResourceManager::IsRegistered(ResourceID id) const {
            return m_IDManager.IsRegistered(id);
        }

        bool ResourceManager::IsPathRegistered(const std::string& filepath) const {
            return m_IDManager.IsPathRegistered(filepath);
        }

        bool ResourceManager::LoadManifest(const std::string& manifestPath) {
            return m_IDManager.LoadManifest(manifestPath);
        }

        bool ResourceManager::SaveManifest(const std::string& manifestPath) const {
            return m_IDManager.SaveManifest(manifestPath);
        }

        void ResourceManager::Clear() {
            // Delete all meshes
            for (auto& pair : m_LoadedMeshes) {
                delete pair.second;
            }
            m_LoadedMeshes.clear();
            
            // Delete all materials
            for (auto& pair : m_LoadedMaterials) {
                delete pair.second;
            }
            m_LoadedMaterials.clear();
            
            // Delete all textures
            for (auto& pair : m_LoadedTextures) {
                delete pair.second;
            }
            m_LoadedTextures.clear();
            
            // Delete all models
            for (auto& pair : m_LoadedModels) {
                delete pair.second;
            }
            m_LoadedModels.clear();
            
            m_PathToIDCache.clear();
        }

    } // namespace Resources
} // namespace FirstEngine
