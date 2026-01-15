#include "FirstEngine/Resources/MaterialLoader.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#include <algorithm>
#include <cctype>

namespace FirstEngine {
    namespace Resources {

        MaterialLoader::LoadResult MaterialLoader::Load(ResourceID id) {
            LoadResult result;
            result.success = false;

            // Get ResourceManager singleton internally
            ResourceManager& resourceManager = ResourceManager::GetInstance();

            // Get resolved path from ResourceManager (internal cache usage)
            std::string resolvedPath = resourceManager.GetResolvedPath(id);
            if (resolvedPath.empty()) {
                return result;
            }

            // Check cache first (ResourceManager internal cache)
            ResourceHandle cached = resourceManager.Get(id);
            if (cached.material) {
                // Return cached data
                IMaterial* material = cached.material;
                result.metadata = material->GetMetadata();
                result.shaderName = material->GetShaderName();
                
                // Copy parameters (would need to reconstruct from parameter data)
                // For now, just mark success
                result.success = true;
                return result;
            }

            // Resolve XML file path
            std::string xmlFilePath = resolvedPath;
            std::string ext = fs::path(xmlFilePath).extension().string();
            if (ext != ".xml" && ext != ".mat") {
                xmlFilePath = fs::path(xmlFilePath).replace_extension(".xml").string();
            }

            // Parse XML file
            ResourceXMLParser parser;
            if (!parser.ParseFromFile(xmlFilePath)) {
                return result;
            }

            // Get metadata from XML
            result.metadata.name = parser.GetName();
            result.metadata.resourceID = parser.GetResourceID();
            result.metadata.filePath = resolvedPath; // Internal use only
            result.metadata.dependencies = parser.GetDependencies();
            result.metadata.isLoaded = false; // Will be set to true after successful load

            // Get material-specific data from XML
            ResourceXMLParser::MaterialData materialData;
            if (!parser.GetMaterialData(materialData)) {
                return result;
            }

            // Set Handle data (only current resource data, not dependencies)
            result.shaderName = materialData.shaderName;
            result.parameters = materialData.parameters;

            // Convert textureSlots to dependencies (texture slots are stored in Textures node)
            // Dependencies will be loaded by Resource::Load, Loader only records them
            for (const auto& pair : materialData.textureSlots) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Texture;
                dep.resourceID = pair.second;
                dep.slot = pair.first;
                dep.isRequired = true;
                result.metadata.dependencies.push_back(dep);
            }

            result.metadata.isLoaded = true;
            result.success = true;
            return result;
        }

        bool MaterialLoader::Save(const std::string& xmlFilePath,
                                  const std::string& name,
                                  ResourceID id,
                                  const std::string& shaderName,
                                  const std::unordered_map<std::string, MaterialParameterValue>& parameters,
                                  const std::unordered_map<std::string, ResourceID>& textureSlots,
                                  const std::vector<ResourceDependency>& dependencies) {
            ResourceXMLParser::MaterialData materialData;
            materialData.shaderName = shaderName;
            materialData.parameters = parameters;
            
            // Convert texture slots map to vector
            std::vector<std::pair<std::string, ResourceID>> textureSlotsVec;
            for (const auto& pair : textureSlots) {
                textureSlotsVec.push_back({pair.first, pair.second});
            }
            materialData.textureSlots = textureSlotsVec;

            return ResourceXMLParser::SaveMaterialToXML(xmlFilePath, name, id, materialData, dependencies);
        }

        bool MaterialLoader::IsFormatSupported(const std::string& filepath) {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".xml" || ext == ".mat";
        }

        std::vector<std::string> MaterialLoader::GetSupportedFormats() {
            return { ".xml", ".mat" };
        }

    } // namespace Resources
} // namespace FirstEngine
