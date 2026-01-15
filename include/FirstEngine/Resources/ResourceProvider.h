#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        // Forward declaration
        class ResourceManager;

        // Base provider interface - provides unified Load and LoadDependencies methods
        // Any resource class should implement both IResource and IResourceProvider
        class FE_RESOURCES_API IResourceProvider {
        public:
            virtual ~IResourceProvider() = default;
            
            // Check if format is supported
            virtual bool IsFormatSupported(const std::string& filepath) const = 0;
            virtual std::vector<std::string> GetSupportedFormats() const = 0;
            
            // Load resource by ID (ONLY method - resource object calls this on itself)
            // Path resolution is completely hidden - ResourceManager singleton handles it internally
            // Loading process MUST be implemented in this order:
            // 1. Parse file to collect dependency ResourceIDs into m_Metadata.dependencies
            //    - Use ResourceManager::GetInstance().GetResolvedPath(id) to get file path (path handling hidden)
            // 2. Call LoadDependencies to recursively load all dependencies
            // 3. Load resource data from file (use GetResolvedPath again if needed)
            // 4. Initialize resource object with loaded data and connect dependencies
            virtual ResourceLoadResult Load(ResourceID id) = 0;
            
            // Load resource from memory
            virtual ResourceLoadResult LoadFromMemory(const void* data, size_t size) = 0;
            
            // Load dependencies for this resource (recursively loads dependent resources by ID)
            // This method is called by Load() after dependencies are collected in m_Metadata.dependencies
            // Dependencies are loaded through ResourceManager singleton and stored with reference counting
            // All dependencies use ResourceID - no paths exposed
            virtual void LoadDependencies() = 0;
        };

        // Resource manager - manages providers and loaded resources with dependency support
        // Singleton pattern - Resource classes access it globally without needing it as a parameter
        class FE_RESOURCES_API ResourceManager {
        public:
            // Singleton access
            static ResourceManager& GetInstance();
            static void Initialize();
            static void Shutdown();

            // Prevent copying
            ResourceManager(const ResourceManager&) = delete;
            ResourceManager& operator=(const ResourceManager&) = delete;

            ~ResourceManager();

            // Set resource search paths (for resolving relative paths)
            void AddSearchPath(const std::string& path);
            void RemoveSearchPath(const std::string& path);
            const std::vector<std::string>& GetSearchPaths() const { return m_SearchPaths; }

            // Register providers (for format detection and creating resource instances)
            void RegisterProvider(ResourceType type, std::unique_ptr<IResourceProvider> provider);

            // Get ResourceIDManager (for ID management)
            ResourceIDManager& GetIDManager() { return m_IDManager; }
            const ResourceIDManager& GetIDManager() const { return m_IDManager; }

            // Unified load/unload interface - primary methods using ResourceID
            ResourceHandle Load(ResourceID id);
            void Unload(ResourceID id);
            void Unload(ResourceHandle handle);
            ResourceHandle Get(ResourceID id) const;

            // Legacy load/unload interface using file paths (for compatibility)
            // These methods automatically register the path and get/create an ID
            ResourceHandle Load(const std::string& filepath, const std::string& basePath = "");
            ResourceHandle Load(ResourceType type, const std::string& filepath, const std::string& basePath = "");
            void Unload(const std::string& filepath);
            void Unload(ResourceType type, const std::string& filepath);
            ResourceHandle Get(const std::string& filepath) const;
            ResourceHandle Get(ResourceType type, const std::string& filepath) const;

            // Detect resource type from file extension
            ResourceType DetectResourceType(const std::string& filepath) const;

            // Resolve resource path (handles relative paths and search paths)
            std::string ResolveResourcePath(const std::string& path, const std::string& basePath = "") const;

            // Clear all resources
            void Clear();

            // Get statistics
            size_t GetLoadedMeshCount() const { return m_LoadedMeshes.size(); }
            size_t GetLoadedMaterialCount() const { return m_LoadedMaterials.size(); }
            size_t GetLoadedTextureCount() const { return m_LoadedTextures.size(); }
            size_t GetLoadedModelCount() const { return m_LoadedModels.size(); }

            // Internal helper methods for Resource classes (hide path handling)
            // These methods are used by Resource classes to get path information without exposing path handling
            friend class IResourceProvider;
            std::string GetResolvedPath(ResourceID id) const; // Get resolved file path for a ResourceID (internal use)

        private:
            // Private constructor for singleton
            ResourceManager();

            // Internal load method by ID
            ResourceHandle LoadInternal(ResourceID id);

            // Get provider for a resource type (for creating resource instances)
            IResourceProvider* GetProvider(ResourceType type) const;

            // Singleton instance
            static std::unique_ptr<ResourceManager> s_Instance;

            // Resource ID Manager
            ResourceIDManager m_IDManager;

            // Provider storage (for creating resource instances)
            std::unordered_map<ResourceType, std::unique_ptr<IResourceProvider>> m_Providers;

            // Cached resources (global cache) - using ResourceID as key
            std::unordered_map<ResourceID, std::unique_ptr<IMesh>> m_LoadedMeshes;
            std::unordered_map<ResourceID, std::unique_ptr<IMaterial>> m_LoadedMaterials;
            std::unordered_map<ResourceID, std::unique_ptr<ITexture>> m_LoadedTextures;
            std::unordered_map<ResourceID, std::unique_ptr<IModel>> m_LoadedModels;

            // Legacy path-based cache (for backwards compatibility)
            std::unordered_map<std::string, ResourceID> m_PathToIDCache;

            // Resource search paths (for resolving relative paths)
            std::vector<std::string> m_SearchPaths;
        };

    } // namespace Resources
} // namespace FirstEngine
