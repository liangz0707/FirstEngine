#include "FirstEngine/Resources/MeshLoader.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ModelLoader.h" // For Vertex and Bone structs
#include "FirstEngine/Resources/ResourceProvider.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <cstring>
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

        MeshLoader::LoadResult MeshLoader::Load(ResourceID id) {
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
            if (cached.mesh) {
                // Return cached data
                IMesh* mesh = cached.mesh;
                result.metadata = mesh->GetMetadata();
                
                // Copy vertex and index data
                size_t vertexDataSize = mesh->GetVertexCount() * mesh->GetVertexStride();
                size_t indexDataSize = mesh->GetIndexCount() * sizeof(uint32_t);
                
                result.vertices.resize(mesh->GetVertexCount());
                result.indices.resize(mesh->GetIndexCount());
                result.vertexStride = mesh->GetVertexStride();
                
                std::memcpy(result.vertices.data(), mesh->GetVertexData(), vertexDataSize);
                std::memcpy(result.indices.data(), mesh->GetIndexData(), indexDataSize);
                
                // Bones are not stored in IMesh interface, so leave empty for cached data
                result.bones.clear();
                
                result.success = true;
                return result;
            }

            // Resolve XML file path
            std::string xmlFilePath = resolvedPath;
            std::string ext = fs::path(xmlFilePath).extension().string();
            if (ext != ".xml" && ext != ".mesh") {
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

            // Get mesh-specific data from XML
            ResourceXMLParser::MeshData meshData;
            if (!parser.GetMeshData(meshData)) {
                return result;
            }

            result.vertexStride = meshData.vertexStride > 0 ? meshData.vertexStride : sizeof(Vertex);

            // Load actual mesh geometry data from source file (fbx, obj, etc.)
            if (!meshData.meshFile.empty()) {
                // Store source mesh file path (relative path from XML)
                result.meshFile = meshData.meshFile;
                
                // Resolve mesh file path relative to XML file directory
                std::string xmlDir = fs::path(xmlFilePath).parent_path().string();
                std::string meshPath = meshData.meshFile;
                
                if (!fs::path(meshPath).is_absolute()) {
                    meshPath = (fs::path(xmlDir) / meshPath).string();
                }

                // Load mesh data directly using Assimp (not through ModelLoader)
                // MeshLoader loads a single mesh from the file
                Assimp::Importer importer;
                const aiScene* scene = importer.ReadFile(
                    meshPath,
                    aiProcess_Triangulate |
                    aiProcess_GenSmoothNormals |
                    aiProcess_FlipUVs |
                    aiProcess_CalcTangentSpace
                );

                if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
                    return result; // Failed to load mesh file
                }

                // Load the first mesh from the file (Mesh resource contains a single mesh)
                if (scene->mNumMeshes > 0) {
                    aiMesh* aiMesh = scene->mMeshes[0];

                    // Load vertices
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

                        result.vertices.push_back(vertex);
                    }

                    // Load indices
                    for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
                        aiFace face = aiMesh->mFaces[j];
                        for (unsigned int k = 0; k < face.mNumIndices; k++) {
                            result.indices.push_back(face.mIndices[k]);
                        }
                    }

                    // Load bone data from this mesh
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
                            result.bones.push_back(bone);
                        }
                    }
                }
            } else {
                // No mesh file specified, return empty result
                return result;
            }

            result.metadata.isLoaded = true;
            result.success = true;
            return result;
        }

        bool MeshLoader::Save(const std::string& xmlFilePath,
                             const std::string& name,
                             ResourceID id,
                             const std::string& meshFile,
                             uint32_t vertexStride) {
            // Save XML with mesh file path (similar to TextureLoader::Save)
            ResourceXMLParser::MeshData meshData;
            meshData.meshFile = meshFile;  // Source mesh file path (relative or absolute)
            meshData.vertexStride = vertexStride > 0 ? vertexStride : sizeof(Vertex);

            return ResourceXMLParser::SaveMeshToXML(xmlFilePath, name, id, meshData);
        }

        bool MeshLoader::IsFormatSupported(const std::string& filepath) {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".xml" || ext == ".mesh";
        }

        std::vector<std::string> MeshLoader::GetSupportedFormats() {
            return { ".xml", ".mesh" };
        }

    } // namespace Resources
} // namespace FirstEngine
