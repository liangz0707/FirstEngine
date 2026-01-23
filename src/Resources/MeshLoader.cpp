#include "FirstEngine/Resources/MeshLoader.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ModelLoader.h" // For Bone struct
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/VertexFormat.h"
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
                std::cerr << "MeshLoader::Load: Failed to get resolved path for mesh ID " << id << std::endl;
                std::cerr << "  Make sure the mesh is registered in resource_manifest.json" << std::endl;
                return result;
            }

            // NOTE: Do NOT check cache here - ResourceManager handles caching
            // If we check cache here, we might return incomplete metadata (before dependencies are loaded)
            // ResourceManager::LoadInternal stores resource in cache BEFORE calling Resource::Load
            // to prevent circular dependencies, but the resource is not fully loaded yet
            // Cache checking should only happen at ResourceManager level, not in Loader

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

                    // Detect available vertex attributes from mesh file
                    bool hasNormals = (aiMesh->mNormals != nullptr);
                    bool hasTexCoords0 = (aiMesh->mTextureCoords[0] != nullptr);
                    bool hasTexCoords1 = (aiMesh->mTextureCoords[1] != nullptr);
                    bool hasTangents = (aiMesh->mTangents != nullptr);
                    bool hasColors0 = (aiMesh->mColors[0] != nullptr);

                    // Create vertex format based on available data
                    result.vertexFormat = VertexFormat::CreateFromMeshData(
                        hasNormals, hasTexCoords0, hasTexCoords1, hasTangents, hasColors0);

                    result.vertexCount = aiMesh->mNumVertices;
                    uint32_t vertexStride = result.vertexFormat.GetStride();
                    result.vertexData.resize(result.vertexCount * vertexStride);

                    // Load vertices into flexible format
                    for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                        uint8_t* vertexPtr = result.vertexData.data() + (j * vertexStride);

                        // Write attributes based on format
                        const auto& attributes = result.vertexFormat.GetAttributes();
                        for (const auto& attr : attributes) {
                            uint8_t* attrPtr = vertexPtr + attr.offset;
                            
                            switch (attr.type) {
                                case VertexAttributeType::Position: {
                                    glm::vec3 pos(
                                        aiMesh->mVertices[j].x,
                                        aiMesh->mVertices[j].y,
                                        aiMesh->mVertices[j].z
                                    );
                                    std::memcpy(attrPtr, &pos, sizeof(glm::vec3));
                                    break;
                                }
                                case VertexAttributeType::Normal: {
                                    glm::vec3 normal;
                                    if (aiMesh->mNormals) {
                                        normal = glm::vec3(
                                            aiMesh->mNormals[j].x,
                                            aiMesh->mNormals[j].y,
                                            aiMesh->mNormals[j].z
                                        );
                                    } else {
                                        normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default
                                    }
                                    std::memcpy(attrPtr, &normal, sizeof(glm::vec3));
                                    break;
                                }
                                case VertexAttributeType::TexCoord0: {
                                    glm::vec2 texCoord;
                                    if (aiMesh->mTextureCoords[0]) {
                                        texCoord = glm::vec2(
                                            aiMesh->mTextureCoords[0][j].x,
                                            aiMesh->mTextureCoords[0][j].y
                                        );
                                    } else {
                                        texCoord = glm::vec2(0.0f, 0.0f); // Default
                                    }
                                    std::memcpy(attrPtr, &texCoord, sizeof(glm::vec2));
                                    break;
                                }
                                case VertexAttributeType::TexCoord1: {
                                    glm::vec2 texCoord;
                                    if (aiMesh->mTextureCoords[1]) {
                                        texCoord = glm::vec2(
                                            aiMesh->mTextureCoords[1][j].x,
                                            aiMesh->mTextureCoords[1][j].y
                                        );
                                    } else {
                                        texCoord = glm::vec2(0.0f, 0.0f); // Default
                                    }
                                    std::memcpy(attrPtr, &texCoord, sizeof(glm::vec2));
                                    break;
                                }
                                case VertexAttributeType::Tangent: {
                                    glm::vec4 tangent;
                                    if (aiMesh->mTangents) {
                                        tangent = glm::vec4(
                                            aiMesh->mTangents[j].x,
                                            aiMesh->mTangents[j].y,
                                            aiMesh->mTangents[j].z,
                                            1.0f  // Default handedness
                                        );
                                        
                                        // Use bitangent to determine handedness if available
                                        if (aiMesh->mBitangents) {
                                            glm::vec3 normal = aiMesh->mNormals ? 
                                                glm::vec3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z) :
                                                glm::vec3(0.0f, 1.0f, 0.0f);
                                            glm::vec3 tan = glm::vec3(tangent);
                                            glm::vec3 bitangent = glm::vec3(
                                                aiMesh->mBitangents[j].x,
                                                aiMesh->mBitangents[j].y,
                                                aiMesh->mBitangents[j].z
                                            );
                                            
                                            glm::vec3 calculatedBitangent = glm::cross(normal, tan);
                                            float handedness = (glm::dot(calculatedBitangent, bitangent) < 0.0f) ? -1.0f : 1.0f;
                                            tangent.w = handedness;
                                        }
                                    } else {
                                        // Calculate tangent if not present
                                        glm::vec3 normal = aiMesh->mNormals ? 
                                            glm::vec3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z) :
                                            glm::vec3(0.0f, 1.0f, 0.0f);
                                        glm::vec3 tan = glm::vec3(1.0f, 0.0f, 0.0f);
                                        
                                        if (glm::abs(glm::dot(normal, tan)) > 0.9f) {
                                            tan = glm::vec3(0.0f, 1.0f, 0.0f);
                                        }
                                        tan = glm::normalize(tan - glm::dot(tan, normal) * normal);
                                        tangent = glm::vec4(tan, 1.0f);
                                    }
                                    std::memcpy(attrPtr, &tangent, sizeof(glm::vec4));
                                    break;
                                }
                                case VertexAttributeType::Color0: {
                                    glm::vec4 color;
                                    if (aiMesh->mColors[0]) {
                                        color = glm::vec4(
                                            aiMesh->mColors[0][j].r,
                                            aiMesh->mColors[0][j].g,
                                            aiMesh->mColors[0][j].b,
                                            aiMesh->mColors[0][j].a
                                        );
                                    } else {
                                        color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
                                    }
                                    std::memcpy(attrPtr, &color, sizeof(glm::vec4));
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
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
                             const std::string& meshFile) {
            // Save XML with mesh file path (similar to TextureLoader::Save)
            // Note: vertexStride is no longer stored - it's calculated from mesh file
            ResourceXMLParser::MeshData meshData;
            meshData.meshFile = meshFile;  // Source mesh file path (relative or absolute)

            return ResourceXMLParser::SaveMeshToXML(xmlFilePath, name, id, meshData);
        }

        std::vector<Vertex> MeshLoader::LoadResult::GetLegacyVertices() const {
            std::vector<Vertex> legacyVertices;
            
            // Only convert if format matches legacy Vertex structure
            if (!vertexFormat.HasAttribute(VertexAttributeType::Position) ||
                !vertexFormat.HasAttribute(VertexAttributeType::Normal) ||
                !vertexFormat.HasAttribute(VertexAttributeType::TexCoord0) ||
                !vertexFormat.HasAttribute(VertexAttributeType::Tangent)) {
                // Format doesn't match legacy structure
                return legacyVertices;
            }

            uint32_t stride = vertexFormat.GetStride();
            legacyVertices.resize(vertexCount);

            for (uint32_t i = 0; i < vertexCount; ++i) {
                const uint8_t* vertexPtr = vertexData.data() + (i * stride);
                
                // Extract position
                if (const auto* posAttr = vertexFormat.GetAttribute(VertexAttributeType::Position)) {
                    std::memcpy(&legacyVertices[i].position, 
                               vertexPtr + posAttr->offset, sizeof(glm::vec3));
                }
                
                // Extract normal
                if (const auto* normAttr = vertexFormat.GetAttribute(VertexAttributeType::Normal)) {
                    std::memcpy(&legacyVertices[i].normal, 
                               vertexPtr + normAttr->offset, sizeof(glm::vec3));
                }
                
                // Extract texCoord
                if (const auto* texAttr = vertexFormat.GetAttribute(VertexAttributeType::TexCoord0)) {
                    std::memcpy(&legacyVertices[i].texCoord, 
                               vertexPtr + texAttr->offset, sizeof(glm::vec2));
                }
                
                // Extract tangent
                if (const auto* tanAttr = vertexFormat.GetAttribute(VertexAttributeType::Tangent)) {
                    std::memcpy(&legacyVertices[i].tangent, 
                               vertexPtr + tanAttr->offset, sizeof(glm::vec4));
                }
            }

            return legacyVertices;
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
