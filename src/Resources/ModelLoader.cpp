#include "FirstEngine/Resources/ModelLoader.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <iostream>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        ModelLoader::LoadResult ModelLoader::Load(ResourceID id) {
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
            if (cached.model) {
                // Return cached data - dependencies are stored in metadata.dependencies
                IModel* model = cached.model;
                result.metadata = model->GetMetadata();
                result.success = true;
                return result;
            }

            // Resolve XML file path
            std::string xmlFilePath = resolvedPath;
            std::string ext = fs::path(xmlFilePath).extension().string();
            if (ext != ".xml") {
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

            // Get model-specific data from XML (only ResourceID references, no actual geometry)
            ResourceXMLParser::ModelData modelData;
            if (!parser.GetModelData(modelData)) {
                return result;
            }

            // Convert ResourceID references to dependencies (stored in metadata)
            // Dependencies will be loaded by Resource::Load, Loader only records them
            for (const auto& pair : modelData.meshIndices) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Mesh;
                dep.resourceID = pair.second;
                dep.slot = std::to_string(pair.first); // Use index as slot to identify mesh position
                dep.isRequired = true;
                result.metadata.dependencies.push_back(dep);
            }

            for (const auto& pair : modelData.materialIndices) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Material;
                dep.resourceID = pair.second;
                dep.slot = std::to_string(pair.first); // Use index as slot to identify material position
                dep.isRequired = true;
                result.metadata.dependencies.push_back(dep);
            }

            for (const auto& pair : modelData.textureSlots) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Texture;
                dep.resourceID = pair.second;
                dep.slot = pair.first; // Use slot name
                dep.isRequired = true;
                result.metadata.dependencies.push_back(dep);
            }

            result.metadata.isLoaded = true;
            result.success = true;
            return result;
        }

        Model ModelLoader::LoadFromFile(const std::string& filepath) {
            Model model;
            model.name = filepath;

            Assimp::Importer importer;

            const aiScene* scene = importer.ReadFile(
                filepath,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                throw std::runtime_error("Failed to load model: " + filepath + " - " + importer.GetErrorString());
            }

            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                Mesh mesh;

                for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                    Vertex vertex;

                    vertex.position = glm::vec3(
                        aiMesh->mVertices[j].x,
                        aiMesh->mVertices[j].y,
                        aiMesh->mVertices[j].z
                    );

                    if (aiMesh->mNormals) {
                        vertex.normal = glm::vec3(
                            aiMesh->mNormals[j].x,
                            aiMesh->mNormals[j].y,
                            aiMesh->mNormals[j].z
                        );
                    }

                    if (aiMesh->mTextureCoords[0]) {
                        vertex.texCoord = glm::vec2(
                            aiMesh->mTextureCoords[0][j].x,
                            aiMesh->mTextureCoords[0][j].y
                        );
                    }

                    mesh.vertices.push_back(vertex);
                }

                for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
                    aiFace face = aiMesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++) {
                        mesh.indices.push_back(face.mIndices[k]);
                    }
                }

                if (aiMesh->mMaterialIndex >= 0) {
                    aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
                    aiString matName;
                    material->Get(AI_MATKEY_NAME, matName);
                    mesh.materialName = matName.C_Str();
                }

                model.meshes.push_back(mesh);
            }

            // Process bone information
            // Note: This is a simplified version, complete skeletal animation requires more complex processing
            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                if (aiMesh->mBones) {
                    for (unsigned int j = 0; j < aiMesh->mNumBones; j++) {
                        aiBone* aiBone = aiMesh->mBones[j];
                        Bone bone;
                        bone.name = aiBone->mName.C_Str();

                        // Convert offset matrix
                        aiMatrix4x4 offsetMatrix = aiBone->mOffsetMatrix;
                        bone.offsetMatrix = glm::mat4(
                            offsetMatrix.a1, offsetMatrix.b1, offsetMatrix.c1, offsetMatrix.d1,
                            offsetMatrix.a2, offsetMatrix.b2, offsetMatrix.c2, offsetMatrix.d2,
                            offsetMatrix.a3, offsetMatrix.b3, offsetMatrix.c3, offsetMatrix.d3,
                            offsetMatrix.a4, offsetMatrix.b4, offsetMatrix.c4, offsetMatrix.d4
                        );

                        bone.parentIndex = -1; // Need to determine from scene graph
                        model.bones.push_back(bone);
                    }
                }
            }

            return model;
        }

        bool ModelLoader::IsFormatSupported(const std::string& filepath) {
            Assimp::Importer importer;
            return importer.IsExtensionSupported(filepath.substr(filepath.find_last_of('.') + 1));
        }

        bool ModelLoader::Save(const std::string& xmlFilePath,
                              const std::string& name,
                              ResourceID id,
                              const std::vector<std::pair<uint32_t, ResourceID>>& meshIndices,
                              const std::vector<std::pair<uint32_t, ResourceID>>& materialIndices,
                              const std::vector<std::pair<std::string, ResourceID>>& textureSlots,
                              const std::vector<ResourceDependency>& dependencies) {
            // Convert meshIndices, materialIndices, textureSlots to dependencies
            // These will be saved in XML and loaded as dependencies
            std::vector<ResourceDependency> allDependencies = dependencies;
            
            for (const auto& pair : meshIndices) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Mesh;
                dep.resourceID = pair.second;
                dep.slot = std::to_string(pair.first);
                dep.isRequired = true;
                allDependencies.push_back(dep);
            }

            for (const auto& pair : materialIndices) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Material;
                dep.resourceID = pair.second;
                dep.slot = std::to_string(pair.first);
                dep.isRequired = true;
                allDependencies.push_back(dep);
            }

            for (const auto& pair : textureSlots) {
                ResourceDependency dep;
                dep.type = ResourceDependency::DependencyType::Texture;
                dep.resourceID = pair.second;
                dep.slot = pair.first;
                dep.isRequired = true;
                allDependencies.push_back(dep);
            }

            // Save to XML (ModelData is still used for XML structure, but dependencies are the source of truth)
            ResourceXMLParser::ModelData modelData;
            modelData.modelFile = ""; // Model is logical, no source file needed
            modelData.meshIndices = meshIndices;  // For XML structure compatibility
            modelData.materialIndices = materialIndices;  // For XML structure compatibility
            modelData.textureSlots = textureSlots;  // For XML structure compatibility

            return ResourceXMLParser::SaveModelToXML(xmlFilePath, name, id, modelData, allDependencies);
        }

        std::vector<std::string> ModelLoader::GetSupportedFormats() {
            Assimp::Importer importer;
            aiString extensions;
            importer.GetExtensionList(extensions);

            std::vector<std::string> formats;
            formats.push_back(".xml"); // Add XML support
            
            std::string extStr = extensions.C_Str();
            std::string delimiter = ";";

            size_t pos = 0;
            std::string token;
            while ((pos = extStr.find(delimiter)) != std::string::npos) {
                token = extStr.substr(0, pos);
                if (!token.empty() && token[0] == '*') {
                    token = token.substr(1);
                }
                formats.push_back(token);
                extStr.erase(0, pos + delimiter.length());
            }
            if (!extStr.empty()) {
                if (extStr[0] == '*') {
                    extStr = extStr.substr(1);
                }
                formats.push_back(extStr);
            }

            return formats;
        }
    }
}
