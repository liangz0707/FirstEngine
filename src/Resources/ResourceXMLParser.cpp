#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include "../../third_party/assimp/contrib/pugixml/src/pugixml.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <glm/glm.hpp>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        ResourceXMLParser::ResourceXMLParser() : m_Document(std::make_unique<pugi::xml_document>()) {
        }

        ResourceXMLParser::~ResourceXMLParser() = default;

        bool ResourceXMLParser::ParseFromFile(const std::string& xmlFilePath) {
            if (!fs::exists(xmlFilePath)) {
                return false;
            }

            pugi::xml_parse_result result = m_Document->load_file(xmlFilePath.c_str());
            if (!result) {
                return false;
            }

            // Get the document element (root element, not the document node itself)
            m_RootNode = m_Document->document_element();
            if (m_RootNode.empty()) {
                // Fallback to root() if document_element() is empty
                m_RootNode = m_Document->root();
            }
            return !m_RootNode.empty();
        }

        bool ResourceXMLParser::ParseFromString(const std::string& xmlContent) {
            pugi::xml_parse_result result = m_Document->load_string(xmlContent.c_str());
            if (!result) {
                return false;
            }

            // Get the document element (root element, not the document node itself)
            m_RootNode = m_Document->document_element();
            if (m_RootNode.empty()) {
                // Fallback to root() if document_element() is empty
                m_RootNode = m_Document->root();
            }
            return !m_RootNode.empty();
        }

        std::string ResourceXMLParser::GetName() const {
            if (m_RootNode.empty()) return "";
            auto nameNode = m_RootNode.child("Name");
            if (nameNode) {
                std::string name = nameNode.text().as_string();
                // Trim whitespace
                if (!name.empty()) {
                    size_t start = name.find_first_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        size_t end = name.find_last_not_of(" \t\n\r");
                        return name.substr(start, end - start + 1);
                    }
                }
                return name;
            }
            return "";
        }

        ResourceID ResourceXMLParser::GetResourceID() const {
            if (m_RootNode.empty()) return InvalidResourceID;
            // Try ResourceID first (preferred for Model, Mesh, Texture)
            auto idNode = m_RootNode.child("ResourceID");
            if (idNode) {
                return ParseResourceID(idNode.text().as_string());
            }
            // Fallback to ID (for backward compatibility with Material, etc.)
            idNode = m_RootNode.child("ID");
            if (idNode) {
                return ParseResourceID(idNode.text().as_string());
            }
            return InvalidResourceID;
        }

        uint64_t ResourceXMLParser::GetFileSize() const {
            if (m_RootNode.empty()) return 0;
            auto sizeNode = m_RootNode.child("FileSize");
            return sizeNode ? sizeNode.text().as_ullong(0) : 0;
        }

        bool ResourceXMLParser::GetIsLoaded() const {
            if (m_RootNode.empty()) return false;
            auto loadedNode = m_RootNode.child("IsLoaded");
            return loadedNode ? loadedNode.text().as_bool(false) : false;
        }

        std::vector<ResourceDependency> ResourceXMLParser::GetDependencies() const {
            std::vector<ResourceDependency> dependencies;
            if (m_RootNode.empty()) return dependencies;

            auto depsNode = m_RootNode.child("Dependencies");
            if (depsNode.empty()) return dependencies;

            for (auto depNode : depsNode.children("Dependency")) {
                ResourceDependency dep;
                
                std::string typeStr = depNode.attribute("type").as_string();
                if (typeStr == "Texture") {
                    dep.type = ResourceDependency::DependencyType::Texture;
                } else if (typeStr == "Material") {
                    dep.type = ResourceDependency::DependencyType::Material;
                } else if (typeStr == "Mesh") {
                    dep.type = ResourceDependency::DependencyType::Mesh;
                } else if (typeStr == "Model") {
                    dep.type = ResourceDependency::DependencyType::Model;
                } else {
                    continue;
                }

                dep.resourceID = ParseResourceID(depNode.attribute("resourceID").as_string());
                dep.slot = depNode.attribute("slot").as_string();
                dep.isRequired = depNode.attribute("required").as_bool(true);

                if (dep.resourceID != InvalidResourceID) {
                    dependencies.push_back(dep);
                }
            }

            return dependencies;
        }

        bool ResourceXMLParser::GetTextureData(TextureData& outData) const {
            if (m_RootNode.empty() || m_RootNode.name() != std::string("Texture")) {
                return false;
            }

            auto imageFileNode = m_RootNode.child("ImageFile");
            if (imageFileNode) {
                outData.imageFile = imageFileNode.text().as_string();
            }

            auto widthNode = m_RootNode.child("Width");
            if (widthNode) {
                outData.width = widthNode.text().as_uint(0);
            }

            auto heightNode = m_RootNode.child("Height");
            if (heightNode) {
                outData.height = heightNode.text().as_uint(0);
            }

            auto channelsNode = m_RootNode.child("Channels");
            if (channelsNode) {
                outData.channels = channelsNode.text().as_uint(0);
            }

            auto hasAlphaNode = m_RootNode.child("HasAlpha");
            if (hasAlphaNode) {
                outData.hasAlpha = hasAlphaNode.text().as_bool(false);
            }

            return true;
        }

        bool ResourceXMLParser::GetMaterialData(MaterialData& outData) const {
            if (m_RootNode.empty() || m_RootNode.name() != std::string("Material")) {
                return false;
            }

            auto shaderNode = m_RootNode.child("Shader");
            if (shaderNode) {
                outData.shaderName = shaderNode.text().as_string();
            }

            // Parse parameters
            auto paramsNode = m_RootNode.child("Parameters");
            if (!paramsNode.empty()) {
                for (pugi::xml_node paramNode : paramsNode.children("Parameter")) {
                    std::string name = paramNode.attribute("name").as_string();
                    std::string type = paramNode.attribute("type").as_string();
                    std::string value = paramNode.text().as_string();
                    
                    if (!name.empty() && !type.empty()) {
                        outData.parameters[name] = this->ParseParameterValue(type, value);
                    }
                }
            }

            // Parse textures
            auto texturesNode = m_RootNode.child("Textures");
            if (!texturesNode.empty()) {
                for (pugi::xml_node texNode : texturesNode.children("Texture")) {
                    std::string slot = texNode.attribute("slot").as_string();
                    ResourceID id = ParseResourceID(texNode.attribute("resourceID").as_string());
                    if (!slot.empty() && id != InvalidResourceID) {
                        outData.textureSlots.push_back({slot, id});
                    }
                }
            }

            return true;
        }

        bool ResourceXMLParser::GetMeshData(MeshData& outData) const {
            if (m_RootNode.empty() || m_RootNode.name() != std::string("Mesh")) {
                return false;
            }

            // Read mesh file path (source file: fbx, obj, etc.)
            auto meshFileNode = m_RootNode.child("MeshFile");
            if (!meshFileNode.empty()) {
                outData.meshFile = meshFileNode.text().as_string();
            }

            // Read vertex stride (optional)
            auto vertexStrideNode = m_RootNode.child("VertexStride");
            if (!vertexStrideNode.empty()) {
                outData.vertexStride = vertexStrideNode.text().as_uint(0);
            }

            return true;
        }

        bool ResourceXMLParser::GetModelData(ModelData& outData) const {
            if (m_RootNode.empty()) {
                return false;
            }
            // Check root node name - use string comparison for safety
            const char* nodeName = m_RootNode.name();
            if (!nodeName || std::string(nodeName) != "Model") {
                return false;
            }

            // ModelFile is deprecated - Model is a logical resource, actual geometry files
            // are specified in Mesh XML files (MeshFile). This is kept for backward compatibility only.
            auto modelFileNode = m_RootNode.child("ModelFile");
            if (!modelFileNode.empty()) {
                outData.modelFile = modelFileNode.text().as_string();
            } else {
                outData.modelFile = ""; // Model is logical, no source file
            }

            // Parse meshes
            auto meshesNode = m_RootNode.child("Meshes");
            if (!meshesNode.empty()) {
                for (pugi::xml_node meshNode : meshesNode.children("Mesh")) {
                    uint32_t index = meshNode.attribute("index").as_uint(0);
                    ResourceID id = ParseResourceID(meshNode.attribute("resourceID").as_string());
                    if (id != InvalidResourceID) {
                        outData.meshIndices.push_back({index, id});
                    }
                }
            }

            // Parse materials
            auto materialsNode = m_RootNode.child("Materials");
            if (!materialsNode.empty()) {
                for (pugi::xml_node matNode : materialsNode.children("Material")) {
                    uint32_t index = matNode.attribute("index").as_uint(0);
                    ResourceID id = ParseResourceID(matNode.attribute("resourceID").as_string());
                    if (id != InvalidResourceID) {
                        outData.materialIndices.push_back({index, id});
                    }
                }
            }

            // Parse textures
            auto texturesNode = m_RootNode.child("Textures");
            if (!texturesNode.empty()) {
                for (pugi::xml_node texNode : texturesNode.children("Texture")) {
                    std::string slot = texNode.attribute("slot").as_string();
                    ResourceID id = ParseResourceID(texNode.attribute("resourceID").as_string());
                    if (!slot.empty() && id != InvalidResourceID) {
                        outData.textureSlots.push_back({slot, id});
                    }
                }
            }

            return true;
        }

        ResourceID ResourceXMLParser::ParseResourceID(const std::string& str) const {
            if (str.empty()) return InvalidResourceID;
            try {
                return std::stoull(str);
            } catch (...) {
                return InvalidResourceID;
            }
        }

        MaterialParameterValue ResourceXMLParser::ParseParameterValue(const std::string& type, const std::string& value) const {
            std::istringstream iss(value);
            
            if (type == "float") {
                float f;
                if (iss >> f) {
                    return MaterialParameterValue(f);
                }
            } else if (type == "vec2") {
                float x, y;
                if (iss >> x >> y) {
                    return MaterialParameterValue(glm::vec2(x, y));
                }
            } else if (type == "vec3") {
                float x, y, z;
                if (iss >> x >> y >> z) {
                    return MaterialParameterValue(glm::vec3(x, y, z));
                }
            } else if (type == "vec4") {
                float x, y, z, w;
                if (iss >> x >> y >> z >> w) {
                    return MaterialParameterValue(glm::vec4(x, y, z, w));
                }
            } else if (type == "int") {
                int32_t i;
                if (iss >> i) {
                    return MaterialParameterValue(i);
                }
            } else if (type == "bool") {
                std::string boolStr = value;
                std::transform(boolStr.begin(), boolStr.end(), boolStr.begin(), ::tolower);
                bool b = (boolStr == "true" || boolStr == "1");
                return MaterialParameterValue(b);
            } else if (type == "mat3") {
                float values[9];
                bool success = true;
                for (int i = 0; i < 9; ++i) {
                    if (!(iss >> values[i])) {
                        success = false;
                        break;
                    }
                }
                if (success) {
                    glm::mat3 mat;
                    for (int i = 0; i < 3; ++i) {
                        for (int j = 0; j < 3; ++j) {
                            mat[i][j] = values[i * 3 + j];
                        }
                    }
                    return MaterialParameterValue(mat);
                }
            } else if (type == "mat4") {
                float values[16];
                bool success = true;
                for (int i = 0; i < 16; ++i) {
                    if (!(iss >> values[i])) {
                        success = false;
                        break;
                    }
                }
                if (success) {
                    glm::mat4 mat;
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 4; ++j) {
                            mat[i][j] = values[i * 4 + j];
                        }
                    }
                    return MaterialParameterValue(mat);
                }
            }

            // Default to float 0.0
            return MaterialParameterValue(0.0f);
        }

        // Save methods
        bool ResourceXMLParser::SaveTextureToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const TextureData& data) {
            pugi::xml_document doc;
            auto root = doc.append_child("Texture");
            
            root.append_child("Name").text().set(name.c_str());
            root.append_child("ResourceID").text().set(std::to_string(id).c_str());
            root.append_child("ImageFile").text().set(data.imageFile.c_str());
            root.append_child("Width").text().set(data.width);
            root.append_child("Height").text().set(data.height);
            root.append_child("Channels").text().set(data.channels);
            root.append_child("HasAlpha").text().set(data.hasAlpha);

            return doc.save_file(xmlFilePath.c_str());
        }

        bool ResourceXMLParser::SaveMaterialToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const MaterialData& data, const std::vector<ResourceDependency>& dependencies) {
            pugi::xml_document doc;
            auto root = doc.append_child("Material");
            
            root.append_child("Name").text().set(name.c_str());
            root.append_child("ResourceID").text().set(std::to_string(id).c_str());
            root.append_child("Shader").text().set(data.shaderName.c_str());

            // Save parameters
            auto paramsNode = root.append_child("Parameters");
            for (auto it = data.parameters.begin(); it != data.parameters.end(); ++it) {
                const auto& pair = *it;
                auto paramNode = paramsNode.append_child("Parameter");
                paramNode.append_attribute("name").set_value(pair.first.c_str());
                
                std::string typeStr;
                switch (pair.second.type) {
                    case MaterialParameterValue::Type::Float: typeStr = "float"; break;
                    case MaterialParameterValue::Type::Vec2: typeStr = "vec2"; break;
                    case MaterialParameterValue::Type::Vec3: typeStr = "vec3"; break;
                    case MaterialParameterValue::Type::Vec4: typeStr = "vec4"; break;
                    case MaterialParameterValue::Type::Int: typeStr = "int"; break;
                    case MaterialParameterValue::Type::Bool: typeStr = "bool"; break;
                    case MaterialParameterValue::Type::Mat3: typeStr = "mat3"; break;
                    case MaterialParameterValue::Type::Mat4: typeStr = "mat4"; break;
                    default: typeStr = "float"; break;
                }
                paramNode.append_attribute("type").set_value(typeStr.c_str());

                // Serialize value to string
                std::ostringstream oss;
                const auto& value = pair.second;
                switch (value.type) {
                    case MaterialParameterValue::Type::Float:
                        oss << value.GetFloat();
                        break;
                    case MaterialParameterValue::Type::Vec2: {
                        glm::vec2 v = value.GetVec2();
                        oss << v.x << " " << v.y;
                        break;
                    }
                    case MaterialParameterValue::Type::Vec3: {
                        glm::vec3 v = value.GetVec3();
                        oss << v.x << " " << v.y << " " << v.z;
                        break;
                    }
                    case MaterialParameterValue::Type::Vec4: {
                        glm::vec4 v = value.GetVec4();
                        oss << v.x << " " << v.y << " " << v.z << " " << v.w;
                        break;
                    }
                    case MaterialParameterValue::Type::Int:
                        oss << value.GetInt();
                        break;
                    case MaterialParameterValue::Type::Bool:
                        oss << (value.GetBool() ? "true" : "false");
                        break;
                    case MaterialParameterValue::Type::Mat3: {
                        glm::mat3 m = value.GetMat3();
                        for (int i = 0; i < 3; ++i) {
                            for (int j = 0; j < 3; ++j) {
                                if (i > 0 || j > 0) oss << " ";
                                oss << m[j][i];  // GLM matrices are column-major: m[column][row]
                            }
                        }
                        break;
                    }
                    case MaterialParameterValue::Type::Mat4: {
                        glm::mat4 m = value.GetMat4();
                        for (int i = 0; i < 4; ++i) {
                            for (int j = 0; j < 4; ++j) {
                                if (i > 0 || j > 0) oss << " ";
                                oss << m[j][i];  // GLM matrices are column-major: m[column][row]
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                paramNode.text().set(oss.str().c_str());
            }

            // Save textures
            auto texturesNode = root.append_child("Textures");
            for (const auto& pair : data.textureSlots) {
                auto texNode = texturesNode.append_child("Texture");
                texNode.append_attribute("slot").set_value(pair.first.c_str());
                texNode.append_attribute("resourceID").set_value(std::to_string(pair.second).c_str());
            }

            // Save dependencies
            if (!dependencies.empty()) {
                auto depsNode = root.append_child("Dependencies");
                for (const auto& dep : dependencies) {
                    auto depNode = depsNode.append_child("Dependency");
                    std::string typeStr;
                    switch (dep.type) {
                        case ResourceDependency::DependencyType::Texture: typeStr = "Texture"; break;
                        case ResourceDependency::DependencyType::Material: typeStr = "Material"; break;
                        case ResourceDependency::DependencyType::Mesh: typeStr = "Mesh"; break;
                        case ResourceDependency::DependencyType::Model: typeStr = "Model"; break;
                        default: continue;
                    }
                    depNode.append_attribute("type").set_value(typeStr.c_str());
                    depNode.append_attribute("resourceID").set_value(std::to_string(dep.resourceID).c_str());
                    if (!dep.slot.empty()) {
                        depNode.append_attribute("slot").set_value(dep.slot.c_str());
                    }
                    depNode.append_attribute("required").set_value(dep.isRequired);
                }
            }

            return doc.save_file(xmlFilePath.c_str());
        }

        bool ResourceXMLParser::SaveMeshToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const MeshData& data) {
            pugi::xml_document doc;
            auto root = doc.append_child("Mesh");
            
            root.append_child("Name").text().set(name.c_str());
            root.append_child("ResourceID").text().set(std::to_string(id).c_str());
            root.append_child("MeshFile").text().set(data.meshFile.c_str());
            if (data.vertexStride > 0) {
                root.append_child("VertexStride").text().set(data.vertexStride);
            }

            return doc.save_file(xmlFilePath.c_str());
        }

        bool ResourceXMLParser::SaveModelToXML(const std::string& xmlFilePath, const std::string& name, ResourceID id, const ModelData& data, const std::vector<ResourceDependency>& dependencies) {
            pugi::xml_document doc;
            auto root = doc.append_child("Model");
            
            root.append_child("Name").text().set(name.c_str());
            root.append_child("ResourceID").text().set(std::to_string(id).c_str());
            // ModelFile is deprecated - Model is a logical resource, actual geometry files
            // are specified in Mesh XML files (MeshFile). Do not save ModelFile.

            // Save meshes
            auto meshesNode = root.append_child("Meshes");
            for (const auto& pair : data.meshIndices) {
                auto meshNode = meshesNode.append_child("Mesh");
                meshNode.append_attribute("index").set_value(pair.first);
                meshNode.append_attribute("resourceID").set_value(std::to_string(pair.second).c_str());
            }

            // Save materials
            auto materialsNode = root.append_child("Materials");
            for (const auto& pair : data.materialIndices) {
                auto matNode = materialsNode.append_child("Material");
                matNode.append_attribute("index").set_value(pair.first);
                matNode.append_attribute("resourceID").set_value(std::to_string(pair.second).c_str());
            }

            // Save textures
            auto texturesNode = root.append_child("Textures");
            for (const auto& pair : data.textureSlots) {
                auto texNode = texturesNode.append_child("Texture");
                texNode.append_attribute("slot").set_value(pair.first.c_str());
                texNode.append_attribute("resourceID").set_value(std::to_string(pair.second).c_str());
            }

            // Save dependencies
            if (!dependencies.empty()) {
                auto depsNode = root.append_child("Dependencies");
                for (const auto& dep : dependencies) {
                    auto depNode = depsNode.append_child("Dependency");
                    std::string typeStr;
                    switch (dep.type) {
                        case ResourceDependency::DependencyType::Texture: typeStr = "Texture"; break;
                        case ResourceDependency::DependencyType::Material: typeStr = "Material"; break;
                        case ResourceDependency::DependencyType::Mesh: typeStr = "Mesh"; break;
                        case ResourceDependency::DependencyType::Model: typeStr = "Model"; break;
                        default: continue;
                    }
                    depNode.append_attribute("type").set_value(typeStr.c_str());
                    depNode.append_attribute("resourceID").set_value(std::to_string(dep.resourceID).c_str());
                    if (!dep.slot.empty()) {
                        depNode.append_attribute("slot").set_value(dep.slot.c_str());
                    }
                    depNode.append_attribute("required").set_value(dep.isRequired);
                }
            }

            return doc.save_file(xmlFilePath.c_str());
        }

    } // namespace Resources
} // namespace FirstEngine
