#pragma once

#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/Export.h"
#include <string>
#include <vector>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Tools {

        class ResourceImport {
        public:
            ResourceImport();
            ~ResourceImport();

            // Parse command line arguments
            bool ParseArguments(int argc, char* argv[]);

            // Execute the import operation
            int Execute();

        private:
            enum class Command {
                Import,
                Help,
                Unknown
            };

            enum class ResourceType {
                Texture,
                Mesh,
                Model,
                Material,
                Unknown
            };

            struct ImportOptions {
                std::string input_file;
                std::string output_dir = "build/Package";
                std::string virtual_path;
                ResourceType resource_type = ResourceType::Unknown;
                std::string name;
                bool overwrite = false;
                bool update_manifest = true;
            };

            Command m_Command;
            ImportOptions m_Options;

            // Helper methods
            void PrintHelp() const;
            ResourceType DetectResourceType(const std::string& filepath) const;
            std::string GenerateVirtualPath(const std::string& filepath, ResourceType type) const;
            std::string GetResourceTypeName(ResourceType type) const;
            std::string GetResourceTypeDirectory(ResourceType type) const;

            // Import methods
            bool ImportTexture(const std::string& inputPath, const ImportOptions& options);
            bool ImportMesh(const std::string& inputPath, const ImportOptions& options);
            bool ImportModel(const std::string& inputPath, const ImportOptions& options);
            bool ImportMaterial(const std::string& inputPath, const ImportOptions& options);

            // Resource ID management

            bool LoadManifest(const std::string& manifestPath);
            bool SaveManifest(const std::string& manifestPath);
            FirstEngine::Resources::ResourceID GenerateResourceID(const std::string& path, FirstEngine::Resources::ResourceType type, const std::string& virtualPath);
            bool RegisterResource(FirstEngine::Resources::ResourceID id, const std::string& path, FirstEngine::Resources::ResourceType type, const std::string& virtualPath);

            // File operations
            bool CopyFileToPackage(const std::string& sourcePath, const std::string& destPath) const;
            std::string GetOutputPath(const std::string& filename, ResourceType type) const;

            // ResourceIDManager instance (we'll use a local instance for import tool)
            FirstEngine::Resources::ResourceIDManager m_IDManager;
        };

    } // namespace Tools
} // namespace FirstEngine
