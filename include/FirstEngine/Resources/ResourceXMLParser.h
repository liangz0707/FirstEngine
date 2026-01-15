#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include "../../third_party/assimp/contrib/pugixml/src/pugixml.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace FirstEngine {
    namespace Resources {

        // XML parser for resource metadata
        // Uses pugixml for XML parsing
        class FE_RESOURCES_API ResourceXMLParser {
        public:
            ResourceXMLParser();
            ~ResourceXMLParser();

            // Parse XML file and extract resource metadata
            bool ParseFromFile(const std::string& xmlFilePath);
            bool ParseFromString(const std::string& xmlContent);

            // Get metadata from parsed XML
            std::string GetName() const;
            ResourceID GetResourceID() const;
            uint64_t GetFileSize() const;
            bool GetIsLoaded() const;

            // Get dependencies
            std::vector<ResourceDependency> GetDependencies() const;

            // Texture-specific data
            struct TextureData {
                std::string imageFile;
                uint32_t width = 0;
                uint32_t height = 0;
                uint32_t channels = 0;
                bool hasAlpha = false;
            };
            bool GetTextureData(TextureData& outData) const;

            // Material-specific data
            struct MaterialData {
                std::string shaderName;
                std::unordered_map<std::string, MaterialParameterValue> parameters;
                std::vector<std::pair<std::string, ResourceID>> textureSlots; // slot name -> ResourceID
            };
            bool GetMaterialData(MaterialData& outData) const;

            // Mesh-specific data
            struct MeshData {
                std::string meshFile;  // Source mesh file (fbx, obj, etc.) - similar to TextureData::imageFile
                uint32_t vertexStride = 0;  // Vertex stride (optional, can be inferred from file)
            };
            bool GetMeshData(MeshData& outData) const;

            // Model-specific data
            struct ModelData {
                std::string modelFile; // Original model file (fbx, obj, etc.)
                std::vector<std::pair<uint32_t, ResourceID>> meshIndices; // mesh index -> ResourceID
                std::vector<std::pair<uint32_t, ResourceID>> materialIndices; // material index -> ResourceID
                std::vector<std::pair<std::string, ResourceID>> textureSlots; // slot name -> ResourceID
            };
            bool GetModelData(ModelData& outData) const;

            // Save resource to XML file
            static bool SaveTextureToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const TextureData& data);
            static bool SaveMaterialToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const MaterialData& data, const std::vector<ResourceDependency>& dependencies);
            static bool SaveMeshToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const MeshData& data);
            static bool SaveModelToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const ModelData& data, const std::vector<ResourceDependency>& dependencies);

        private:
            std::unique_ptr<pugi::xml_document> m_Document;
            pugi::xml_node m_RootNode;

            // Helper methods
            ResourceID ParseResourceID(const std::string& str) const;
            MaterialParameterValue ParseParameterValue(const std::string& type, const std::string& value) const;
        };

    } // namespace Resources
} // namespace FirstEngine
