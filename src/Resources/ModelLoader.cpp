#include "FirstEngine/Resources/ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <iostream>

namespace FirstEngine {
    namespace Resources {
        Model ModelLoader::LoadFromFile(const std::string& filepath) {
            Model model;
            model.name = filepath;

            Assimp::Importer importer;

            // 导入模型，应用各种后处理
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

            // 处理网格
            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                Mesh mesh;

                // 处理顶点
                for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
                    Vertex vertex;

                    // 位置
                    vertex.position = glm::vec3(
                        aiMesh->mVertices[j].x,
                        aiMesh->mVertices[j].y,
                        aiMesh->mVertices[j].z
                    );

                    // 法线
                    if (aiMesh->mNormals) {
                        vertex.normal = glm::vec3(
                            aiMesh->mNormals[j].x,
                            aiMesh->mNormals[j].y,
                            aiMesh->mNormals[j].z
                        );
                    }

                    // 纹理坐标
                    if (aiMesh->mTextureCoords[0]) {
                        vertex.texCoord = glm::vec2(
                            aiMesh->mTextureCoords[0][j].x,
                            aiMesh->mTextureCoords[0][j].y
                        );
                    }

                    mesh.vertices.push_back(vertex);
                }

                // 处理索引
                for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
                    aiFace face = aiMesh->mFaces[j];
                    for (unsigned int k = 0; k < face.mNumIndices; k++) {
                        mesh.indices.push_back(face.mIndices[k]);
                    }
                }

                // 材质名称
                if (aiMesh->mMaterialIndex >= 0) {
                    aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
                    aiString matName;
                    material->Get(AI_MATKEY_NAME, matName);
                    mesh.materialName = matName.C_Str();
                }

                model.meshes.push_back(mesh);
            }

            // 处理骨骼信息
            // 注意：这是一个简化版本，完整的骨骼动画需要更复杂的处理
            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                if (aiMesh->mBones) {
                    for (unsigned int j = 0; j < aiMesh->mNumBones; j++) {
                        aiBone* aiBone = aiMesh->mBones[j];
                        Bone bone;
                        bone.name = aiBone->mName.C_Str();

                        // 转换偏移矩阵
                        aiMatrix4x4 offsetMatrix = aiBone->mOffsetMatrix;
                        bone.offsetMatrix = glm::mat4(
                            offsetMatrix.a1, offsetMatrix.b1, offsetMatrix.c1, offsetMatrix.d1,
                            offsetMatrix.a2, offsetMatrix.b2, offsetMatrix.c2, offsetMatrix.d2,
                            offsetMatrix.a3, offsetMatrix.b3, offsetMatrix.c3, offsetMatrix.d3,
                            offsetMatrix.a4, offsetMatrix.b4, offsetMatrix.c4, offsetMatrix.d4
                        );

                        bone.parentIndex = -1; // 需要从场景图中确定
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

        std::vector<std::string> ModelLoader::GetSupportedFormats() {
            Assimp::Importer importer;
            aiString extensions;
            importer.GetExtensionList(extensions);

            std::vector<std::string> formats;
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
