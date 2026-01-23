#pragma once

// Disable Windows min/max macros before including headers
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceTypes.h" 
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        class ResourceManager;

        class FE_RESOURCES_API IResourceProvider {
        public:
            virtual ~IResourceProvider() = default;

            // Check if format is supported
            virtual bool IsFormatSupported(const std::string& filepath) const = 0;
            virtual std::vector<std::string> GetSupportedFormats() const = 0;

            virtual ResourceLoadResult Load(ResourceID id) = 0;

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

            void AddSearchPath(const std::string& path);
            void RemoveSearchPath(const std::string& path);
            const std::vector<std::string>& GetSearchPaths() const { return m_SearchPaths; }

            // Resource ID management (encapsulates ResourceIDManager functionality)
            ResourceID RegisterResource(const std::string& filepath, ResourceType type, const std::string& virtualPath = "");
            ResourceID GetIDFromPath(const std::string& filepath) const;
            std::string GetPathFromID(ResourceID id) const;
            ResourceType GetTypeFromID(ResourceID id) const;
            bool IsRegistered(ResourceID id) const;
            bool IsPathRegistered(const std::string& filepath) const;
            bool LoadManifest(const std::string& manifestPath);
            bool SaveManifest(const std::string& manifestPath) const;

            // Unified load/unload interface - primary methods using ResourceID
            ResourceHandle Load(ResourceID id);
            void Unload(ResourceID id);
            void Unload(ResourceHandle handle);
            ResourceHandle Get(ResourceID id) const;
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

            // Singleton instance
            static std::unique_ptr<ResourceManager> s_Instance;

            // Resource ID Manager
            ResourceIDManager m_IDManager;

            std::unordered_map<ResourceID, IMesh*> m_LoadedMeshes;
            std::unordered_map<ResourceID, IMaterial*> m_LoadedMaterials;
            std::unordered_map<ResourceID, ITexture*> m_LoadedTextures;
            std::unordered_map<ResourceID, IModel*> m_LoadedModels;

            // Legacy path-based cache (for backward compatibility)
            // NOTE: This cache is kept for backward compatibility with path-based API.
            // TODO: Consider removing this in a future version when all code migrates to ResourceID-based API
            std::unordered_map<std::string, ResourceID> m_PathToIDCache;

            // Resource search paths (for resolving relative paths)
            std::vector<std::string> m_SearchPaths;
        };

    } // namespace Resources
} // namespace FirstEngine
