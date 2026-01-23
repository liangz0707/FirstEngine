#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Core/ConfigFile.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#endif

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Renderer {

        // Singleton instance
        std::unique_ptr<RenderResourceManager> RenderResourceManager::s_Instance = nullptr;

        RenderResourceManager& RenderResourceManager::GetInstance() {
            if (!s_Instance) {
                Initialize();
            }
            return *s_Instance;
        }

        void RenderResourceManager::Initialize() {
            if (!s_Instance) {
                s_Instance = std::unique_ptr<RenderResourceManager>(new RenderResourceManager());
            }
        }

        void RenderResourceManager::Shutdown() {
            if (s_Instance) {
                s_Instance.reset();
            }
        }

        void RenderResourceManager::RegisterResource(IRenderResource* resource) {
            if (!resource) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            // Check if already registered
            auto it = std::find(m_Resources.begin(), m_Resources.end(), resource);
            if (it == m_Resources.end()) {
                m_Resources.push_back(resource);
            }
        }

        void RenderResourceManager::UnregisterResource(IRenderResource* resource) {
            if (!resource) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            auto it = std::find(m_Resources.begin(), m_Resources.end(), resource);
            if (it != m_Resources.end()) {
                m_Resources.erase(it);
            }
        }

        uint32_t RenderResourceManager::ProcessScheduledResources(
            RHI::IDevice* device, 
            uint32_t maxResourcesPerFrame
        ) {
            if (!device) {
                return 0;
            }

            // Get a snapshot of resources (thread-safe)
            std::vector<IRenderResource*> resources;
            {
                std::lock_guard<std::mutex> lock(m_ResourcesMutex);
                resources = m_Resources;
            }

            uint32_t processedCount = 0;
            uint32_t processedThisFrame = 0;

            // Process scheduled resources
            for (IRenderResource* resource : resources) {
                if (!resource) {
                    continue;
                }

                // Check if resource is scheduled
                if (!resource->IsScheduled()) {
                    continue;
                }

                // Process the scheduled operation
                if (resource->ProcessScheduled(device)) {
                    processedCount++;
                    processedThisFrame++;

                    // Limit resources processed per frame
                    if (maxResourcesPerFrame > 0 && processedThisFrame >= maxResourcesPerFrame) {
                        break;
                    }
                }
            }

            return processedCount;
        }

        std::vector<IRenderResource*> RenderResourceManager::GetAllResources() const {
            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            return m_Resources;
        }

        RenderResourceManager::ResourceStatistics RenderResourceManager::GetStatistics() const {
            ResourceStatistics stats;

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            for (IRenderResource* resource : m_Resources) {
                if (!resource) {
                    continue;
                }

                // Get actual resource state
                ResourceState resourceState = resource->GetResourceState();
                switch (resourceState) {
                    case ResourceState::Uninitialized:
                        stats.uninitialized++;
                        break;
                    case ResourceState::Created:
                        stats.created++;
                        break;
                    case ResourceState::Destroyed:
                        stats.destroyed++;
                        break;
                }

                // Get schedule state
                ScheduleState scheduleState = resource->GetScheduleState();
                switch (scheduleState) {
                    case ScheduleState::ScheduledCreate:
                        stats.scheduledCreate++;
                        break;
                    case ScheduleState::ScheduledUpdate:
                        stats.scheduledUpdate++;
                        break;
                    case ScheduleState::ScheduledDestroy:
                        stats.scheduledDestroy++;
                        break;
                    case ScheduleState::None:
                        break;
                }

                // Get operation state
                OperationState operationState = resource->GetOperationState();
                switch (operationState) {
                    case OperationState::Creating:
                        stats.creating++;
                        break;
                    case OperationState::Updating:
                        stats.updating++;
                        break;
                    case OperationState::Destroying:
                        stats.destroying++;
                        break;
                    case OperationState::Idle:
                        break;
                }
            }

            return stats;
        }

        std::string RenderResourceManager::ResolvePath(const std::string& relativePath) {
            // Normalize path separators
            std::string normalizedPath = relativePath;
            std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
            
            // Try current working directory first
            if (fs::exists(normalizedPath)) {
                return fs::absolute(normalizedPath).string();
            }
            
            // Try relative to executable directory (Windows)
#ifdef _WIN32
            char exePath[MAX_PATH];
            DWORD pathLen = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            if (pathLen > 0 && pathLen < MAX_PATH) {
                // Remove executable name, keep directory
                std::string exeDir;
                for (int i = pathLen - 1; i >= 0; i--) {
                    if (exePath[i] == '\\' || exePath[i] == '/') {
                        exePath[i + 1] = '\0';
                        exeDir = exePath;
                        // Normalize to forward slashes for consistency
                        std::replace(exeDir.begin(), exeDir.end(), '\\', '/');
                        break;
                    }
                }
                
                if (!exeDir.empty()) {
                    // Try in executable directory
                    std::string fullPath = exeDir + normalizedPath;
                    if (fs::exists(fullPath)) {
                        return fs::absolute(fullPath).string();
                    }
                    // Try going up one directory (if exe is in build/Debug or build/Release)
                    std::string parentPath = exeDir + "../" + normalizedPath;
                    if (fs::exists(parentPath)) {
                        return fs::absolute(parentPath).string();
                    }
                    // Try going up two directories (if exe is in build/Debug/FirstEngine or similar)
                    std::string grandParentPath = exeDir + "../../" + normalizedPath;
                    if (fs::exists(grandParentPath)) {
                        return fs::absolute(grandParentPath).string();
                    }
                    // Try going up three directories (if exe is deep in build tree)
                    std::string greatGrandParentPath = exeDir + "../../../" + normalizedPath;
                    if (fs::exists(greatGrandParentPath)) {
                        return fs::absolute(greatGrandParentPath).string();
                    }
                }
            }
#endif
            // Return original path if not found (caller will handle error)
            return normalizedPath;
        }

        bool RenderResourceManager::LoadPackageResources(const std::string& configPath) {
            // Load configuration file
            FirstEngine::Core::ConfigFile config;
            std::string resolvedConfigPath = ResolvePath(configPath);
            
            if (!fs::exists(resolvedConfigPath)) {
                std::cerr << "RenderResourceManager: Config file not found: " << resolvedConfigPath << std::endl;
                return false;
            }

            if (!config.LoadFromFile(resolvedConfigPath)) {
                std::cerr << "RenderResourceManager: Failed to load config file: " << resolvedConfigPath << std::endl;
                return false;
            }

            // Get Package path from config
            std::string packagePath = config.GetValue("PackagePath", "");
            if (packagePath.empty()) {
                std::cerr << "RenderResourceManager: PackagePath not found in config file" << std::endl;
                return false;
            }

            // Resolve Package path (might be relative)
            std::string resolvedPackagePath = ResolvePath(packagePath);
            if (!fs::exists(resolvedPackagePath)) {
                std::cerr << "RenderResourceManager: Package directory not found: " << resolvedPackagePath << std::endl;
                return false;
            }

            return LoadPackageResourcesFromPath(resolvedPackagePath);
        }

        bool RenderResourceManager::LoadPackageResourcesFromPath(const std::string& packagePath) {
            // Normalize path
            std::string normalizedPath = packagePath;
            std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
            
            // Ensure path ends with /
            if (!normalizedPath.empty() && normalizedPath.back() != '/') {
                normalizedPath += '/';
            }

            if (!fs::exists(normalizedPath)) {
                std::cerr << "RenderResourceManager: Package directory does not exist: " << normalizedPath << std::endl;
                return false;
            }

            // Initialize ResourceManager if not already initialized
            FirstEngine::Resources::ResourceManager::Initialize();
            FirstEngine::Resources::ResourceManager& resourceManager = FirstEngine::Resources::ResourceManager::GetInstance();

            // Add Package directory as search path
            resourceManager.AddSearchPath(normalizedPath);
            resourceManager.AddSearchPath(normalizedPath + "Models");
            resourceManager.AddSearchPath(normalizedPath + "Materials");
            resourceManager.AddSearchPath(normalizedPath + "Textures");
            resourceManager.AddSearchPath(normalizedPath + "Shaders");
            resourceManager.AddSearchPath(normalizedPath + "Scenes");

            // Initialize ShaderCollectionsTools with shader directory
            auto& collectionsTools = ShaderCollectionsTools::GetInstance();
            std::string shaderDir = normalizedPath + "Shaders";
            if (!collectionsTools.Initialize(shaderDir)) {
                std::cerr << "RenderResourceManager: Warning: Failed to initialize ShaderCollectionsTools" << std::endl;
            } else {
                // Load all shaders from Package/Shaders directory at startup
                if (fs::exists(shaderDir)) {
                    std::cout << "RenderResourceManager: Loading all shaders from: " << shaderDir << std::endl;
                    if (collectionsTools.LoadAllShadersFromDirectory(shaderDir)) {
                        std::cout << "RenderResourceManager: Successfully loaded all shaders" << std::endl;
                    } else {
                        std::cerr << "RenderResourceManager: Warning: Failed to load some shaders" << std::endl;
                    }
                } else {
                    std::cout << "RenderResourceManager: Shaders directory not found, shaders will be loaded on demand" << std::endl;
                }
            }

            // Load resource manifest
            std::string manifestPath = normalizedPath + "resource_manifest.json";
            if (fs::exists(manifestPath)) {
                if (resourceManager.LoadManifest(manifestPath)) {
                    std::cout << "RenderResourceManager: Loaded resource manifest from: " << manifestPath << std::endl;
                } else {
                    std::cerr << "RenderResourceManager: Failed to load resource manifest from: " << manifestPath << std::endl;
                    return false;
                }
            } else {
                std::cout << "RenderResourceManager: Resource manifest not found: " << manifestPath << std::endl;
                std::cout << "RenderResourceManager: Will register resources on demand." << std::endl;
            }

            // Store current package path
            m_CurrentPackagePath = normalizedPath;

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
