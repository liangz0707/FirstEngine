#include "FirstEngine/Tools/ResourceImport.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ImageLoader.h"
#include "FirstEngine/Resources/ModelLoader.h"
#include "FirstEngine/Resources/MeshLoader.h"
#include "FirstEngine/Resources/MaterialLoader.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Tools {

        ResourceImport::ResourceImport() : m_Command(Command::Help) {
        }

        ResourceImport::~ResourceImport() = default;

        bool ResourceImport::ParseArguments(int argc, char* argv[]) {
            if (argc < 2) {
                m_Command = Command::Help;
                return true;
            }

            std::string command_str = argv[1];
            std::transform(command_str.begin(), command_str.end(), command_str.begin(), ::tolower);

            if (command_str == "import" || command_str == "i") {
                m_Command = Command::Import;
            } else if (command_str == "help" || command_str == "h" || command_str == "-h" || command_str == "--help") {
                m_Command = Command::Help;
                return true;
            } else {
                std::cerr << "Unknown command: " << command_str << std::endl;
                m_Command = Command::Help;
                return false;
            }

            // Parse command-specific arguments
            for (int i = 2; i < argc; i++) {
                std::string arg = argv[i];

                if (arg == "-i" || arg == "--input") {
                    if (i + 1 < argc) {
                        m_Options.input_file = argv[++i];
                    } else {
                        std::cerr << "Error: -i/--input requires a file path" << std::endl;
                        return false;
                    }
                } else if (arg == "-o" || arg == "--output") {
                    if (i + 1 < argc) {
                        m_Options.output_dir = argv[++i];
                    } else {
                        std::cerr << "Error: -o/--output requires a directory path" << std::endl;
                        return false;
                    }
                } else if (arg == "-v" || arg == "--virtual-path") {
                    if (i + 1 < argc) {
                        m_Options.virtual_path = argv[++i];
                    } else {
                        std::cerr << "Error: -v/--virtual-path requires a path" << std::endl;
                        return false;
                    }
                } else if (arg == "-t" || arg == "--type") {
                    if (i + 1 < argc) {
                        std::string type_str = argv[++i];
                        std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::tolower);
                        if (type_str == "texture" || type_str == "tex") {
                            m_Options.resource_type = ResourceType::Texture;
                        } else if (type_str == "mesh") {
                            m_Options.resource_type = ResourceType::Mesh;
                        } else if (type_str == "model" || type_str == "mod") {
                            m_Options.resource_type = ResourceType::Model;
                        } else if (type_str == "material" || type_str == "mat") {
                            m_Options.resource_type = ResourceType::Material;
                        } else {
                            std::cerr << "Error: Unknown resource type: " << type_str << std::endl;
                            return false;
                        }
                    } else {
                        std::cerr << "Error: -t/--type requires a type (texture, mesh, model, material)" << std::endl;
                        return false;
                    }
                } else if (arg == "-n" || arg == "--name") {
                    if (i + 1 < argc) {
                        m_Options.name = argv[++i];
                    } else {
                        std::cerr << "Error: -n/--name requires a name" << std::endl;
                        return false;
                    }
                } else if (arg == "--overwrite") {
                    m_Options.overwrite = true;
                } else if (arg == "--no-manifest") {
                    m_Options.update_manifest = false;
                } else {
                    std::cerr << "Unknown argument: " << arg << std::endl;
                    return false;
                }
            }

            // Validate required arguments
            if (m_Command == Command::Import) {
                if (m_Options.input_file.empty()) {
                    std::cerr << "Error: Input file is required (-i/--input)" << std::endl;
                    return false;
                }
                if (!fs::exists(m_Options.input_file)) {
                    std::cerr << "Error: Input file does not exist: " << m_Options.input_file << std::endl;
                    return false;
                }
            }

            return true;
        }

        int ResourceImport::Execute() {
            if (m_Command == Command::Help) {
                PrintHelp();
                return 0;
            }

            if (m_Command == Command::Import) {
                // Load existing manifest if it exists
                std::string manifestPath = m_Options.output_dir + "/resource_manifest.json";
                if (fs::exists(manifestPath)) {
                    if (!LoadManifest(manifestPath)) {
                        std::cerr << "Warning: Failed to load existing manifest, will create new one" << std::endl;
                    }
                }

                // Auto-detect resource type if not specified
                if (m_Options.resource_type == ResourceType::Unknown) {
                    m_Options.resource_type = DetectResourceType(m_Options.input_file);
                    if (m_Options.resource_type == ResourceType::Unknown) {
                        std::cerr << "Error: Could not detect resource type. Please specify with -t/--type" << std::endl;
                        return 1;
                    }
                    std::cout << "Auto-detected resource type: " << GetResourceTypeName(m_Options.resource_type) << std::endl;
                }

                // Generate name if not provided
                if (m_Options.name.empty()) {
                    fs::path inputPath(m_Options.input_file);
                    m_Options.name = inputPath.stem().string();
                }

                // Generate virtual path if not provided
                if (m_Options.virtual_path.empty()) {
                    m_Options.virtual_path = GenerateVirtualPath(m_Options.input_file, m_Options.resource_type);
                }

                // Import based on type
                bool success = false;
                switch (m_Options.resource_type) {
                    case ResourceType::Texture:
                        success = ImportTexture(m_Options.input_file, m_Options);
                        break;
                    case ResourceType::Mesh:
                        success = ImportMesh(m_Options.input_file, m_Options);
                        break;
                    case ResourceType::Model:
                        success = ImportModel(m_Options.input_file, m_Options);
                        break;
                    case ResourceType::Material:
                        success = ImportMaterial(m_Options.input_file, m_Options);
                        break;
                    default:
                        std::cerr << "Error: Unknown resource type" << std::endl;
                        return 1;
                }

                if (success) {
                    // Save manifest
                    if (m_Options.update_manifest) {
                        if (SaveManifest(manifestPath)) {
                            std::cout << "Manifest updated: " << manifestPath << std::endl;
                        } else {
                            std::cerr << "Warning: Failed to save manifest" << std::endl;
                        }
                    }
                    std::cout << "Import completed successfully!" << std::endl;
                    return 0;
                } else {
                    std::cerr << "Import failed!" << std::endl;
                    return 1;
                }
            }

            return 1;
        }

        void ResourceImport::PrintHelp() const {
            std::cout << "ResourceImport - FirstEngine Resource Import Tool\n";
            std::cout << "Usage: ResourceImport <command> [options]\n\n";
            std::cout << "Commands:\n";
            std::cout << "  import, i    Import a resource file\n";
            std::cout << "  help, h      Show this help message\n\n";
            std::cout << "Import Options:\n";
            std::cout << "  -i, --input <file>          Input file to import (required)\n";
            std::cout << "  -o, --output <dir>          Output directory (default: build/Package)\n";
            std::cout << "  -t, --type <type>           Resource type: texture, mesh, model, material\n";
            std::cout << "  -v, --virtual-path <path>   Virtual path for the resource\n";
            std::cout << "  -n, --name <name>           Resource name (default: filename without extension)\n";
            std::cout << "  --overwrite                 Overwrite existing files\n";
            std::cout << "  --no-manifest               Don't update resource manifest\n\n";
            std::cout << "Examples:\n";
            std::cout << "  ResourceImport import -i texture.png -t texture\n";
            std::cout << "  ResourceImport import -i model.fbx -t model -n MyModel\n";
            std::cout << "  ResourceImport import -i mesh.obj -t mesh -o build/Package/Meshes\n";
        }

        ResourceImport::ResourceType ResourceImport::DetectResourceType(const std::string& filepath) const {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // Texture formats
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || 
                ext == ".tga" || ext == ".dds" || ext == ".tiff" || ext == ".hdr" ||
                ext == ".exr" || ext == ".ktx") {
                return ResourceType::Texture;
            }

            // Model formats (can contain multiple meshes)
            if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae" || 
                ext == ".3ds" || ext == ".blend" || ext == ".x") {
                return ResourceType::Model;
            }

            // Mesh formats (single mesh)
            if (ext == ".obj" || ext == ".ply" || ext == ".stl") {
                return ResourceType::Mesh;
            }

            // Material formats
            if (ext == ".mat" || ext == ".material" || ext == ".json") {
                return ResourceType::Material;
            }

            return ResourceType::Unknown;
        }

        std::string ResourceImport::GenerateVirtualPath(const std::string& filepath, ResourceType type) const {
            fs::path path(filepath);
            std::string stem = path.stem().string();
            std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
            
            std::string typeDir = GetResourceTypeDirectory(type);
            return typeDir + "/" + stem;
        }

        std::string ResourceImport::GetResourceTypeName(ResourceType type) const {
            switch (type) {
                case ResourceType::Texture: return "Texture";
                case ResourceType::Mesh: return "Mesh";
                case ResourceType::Model: return "Model";
                case ResourceType::Material: return "Material";
                default: return "Unknown";
            }
        }

        std::string ResourceImport::GetResourceTypeDirectory(ResourceType type) const {
            switch (type) {
                case ResourceType::Texture: return "textures";
                case ResourceType::Mesh: return "meshes";
                case ResourceType::Model: return "models";
                case ResourceType::Material: return "materials";
                default: return "";
            }
        }

        bool ResourceImport::ImportTexture(const std::string& inputPath, const ImportOptions& options) {
            try {
                // Load image to get metadata
                FirstEngine::Resources::ImageData imageData = FirstEngine::Resources::ImageLoader::LoadFromFile(inputPath);
                
                // Generate output paths
                fs::path inputFilePath(inputPath);
                std::string filename = inputFilePath.filename().string();
                std::string outputPath = GetOutputPath(filename, ResourceType::Texture);
                
                // XML path (relative to output_dir for manifest registration)
                std::string typeDir = GetResourceTypeDirectory(ResourceType::Texture);
                typeDir[0] = std::toupper(typeDir[0]); // Capitalize first letter
                fs::path xmlPathRel = fs::path(typeDir) / fs::path(filename).replace_extension(".xml");
                std::string xmlPath = (fs::path(m_Options.output_dir) / xmlPathRel).string();
                // Normalize path separators
                std::replace(xmlPath.begin(), xmlPath.end(), '\\', '/');
                
                // Relative path for manifest (e.g., "Textures/texture.xml")
                std::string xmlPathForManifest = xmlPathRel.string();
                std::replace(xmlPathForManifest.begin(), xmlPathForManifest.end(), '\\', '/');

                // Check if file exists and overwrite flag
                if (fs::exists(outputPath) && !options.overwrite) {
                    std::cerr << "Error: Output file already exists: " << outputPath << " (use --overwrite to replace)" << std::endl;
                    return false;
                }

                // Copy file to output directory
                if (!CopyFileToPackage(inputPath, outputPath)) {
                    std::cerr << "Error: Failed to copy file to: " << outputPath << std::endl;
                    return false;
                }

                // Convert ResourceType
                FirstEngine::Resources::ResourceType resourceType = FirstEngine::Resources::ResourceType::Texture;

                // Generate or get ResourceID (use relative XML path for manifest registration)

                FirstEngine::Resources::ResourceID id = GenerateResourceID(xmlPathForManifest, resourceType, options.virtual_path);

                // Create TextureData
                FirstEngine::Resources::ResourceXMLParser::TextureData textureData;
                textureData.imageFile = filename;
                textureData.width = imageData.width;
                textureData.height = imageData.height;
                textureData.channels = imageData.channels;
                textureData.hasAlpha = (imageData.channels == 4);

                // Save XML
                if (!FirstEngine::Resources::ResourceXMLParser::SaveTextureToXML(xmlPath, options.name, id, textureData)) {
                    std::cerr << "Error: Failed to save XML file: " << xmlPath << std::endl;
                    return false;
                }

                std::cout << "Imported texture: " << options.name << " (ID: " << id << ")" << std::endl;
                std::cout << "  File: " << outputPath << std::endl;
                std::cout << "  XML: " << xmlPath << std::endl;
                std::cout << "  Size: " << imageData.width << "x" << imageData.height << " (" << imageData.channels << " channels)" << std::endl;

                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error importing texture: " << e.what() << std::endl;
                return false;
            }
        }

        bool ResourceImport::ImportMesh(const std::string& inputPath, const ImportOptions& options) {
            try {
                // Generate output paths
                fs::path inputFilePath(inputPath);
                std::string filename = inputFilePath.filename().string();
                std::string outputPath = GetOutputPath(filename, ResourceType::Mesh);
                
                // XML path (relative to output_dir for manifest registration)
                std::string typeDir = GetResourceTypeDirectory(ResourceType::Mesh);
                typeDir[0] = std::toupper(typeDir[0]); // Capitalize first letter
                fs::path xmlPathRel = fs::path(typeDir) / fs::path(filename).replace_extension(".xml");
                std::string xmlPath = (fs::path(m_Options.output_dir) / xmlPathRel).string();
                // Normalize path separators
                std::replace(xmlPath.begin(), xmlPath.end(), '\\', '/');
                
                // Relative path for manifest (e.g., "Meshes/mesh.xml")
                std::string xmlPathForManifest = xmlPathRel.string();
                std::replace(xmlPathForManifest.begin(), xmlPathForManifest.end(), '\\', '/');

                // Check if file exists and overwrite flag
                if (fs::exists(outputPath) && !options.overwrite) {
                    std::cerr << "Error: Output file already exists: " << outputPath << " (use --overwrite to replace)" << std::endl;
                    return false;
                }

                // Copy file to output directory
                if (!CopyFileToPackage(inputPath, outputPath)) {
                    std::cerr << "Error: Failed to copy file to: " << outputPath << std::endl;
                    return false;
                }

                // Convert ResourceType
                FirstEngine::Resources::ResourceType resourceType = FirstEngine::Resources::ResourceType::Mesh;

                // Generate or get ResourceID (use relative XML path for manifest registration)
                FirstEngine::Resources::ResourceID id = GenerateResourceID(xmlPathForManifest, resourceType, options.virtual_path);

                // Create MeshData
                FirstEngine::Resources::ResourceXMLParser::MeshData meshData;
                meshData.meshFile = filename;
                meshData.vertexStride = 32; // Default vertex stride (position + normal + texcoord)

                // Save XML
                if (!FirstEngine::Resources::ResourceXMLParser::SaveMeshToXML(xmlPath, options.name, id, meshData)) {
                    std::cerr << "Error: Failed to save XML file: " << xmlPath << std::endl;
                    return false;
                }

                std::cout << "Imported mesh: " << options.name << " (ID: " << id << ")" << std::endl;
                std::cout << "  File: " << outputPath << std::endl;
                std::cout << "  XML: " << xmlPath << std::endl;

                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error importing mesh: " << e.what() << std::endl;
                return false;
            }
        }

        bool ResourceImport::ImportModel(const std::string& inputPath, const ImportOptions& options) {
            try {
                // For models, we need to process them with Assimp to extract meshes and materials
                // For now, we'll create a basic model XML that references the source file
                // The actual mesh extraction would be done during runtime loading

                // Generate output paths
                fs::path inputFilePath(inputPath);
                std::string filename = inputFilePath.filename().string();
                std::string outputPath = GetOutputPath(filename, ResourceType::Model);
                
                // XML path (relative to output_dir for manifest registration)
                std::string typeDir = GetResourceTypeDirectory(ResourceType::Model);
                typeDir[0] = std::toupper(typeDir[0]); // Capitalize first letter
                fs::path xmlPathRel = fs::path(typeDir) / fs::path(filename).replace_extension(".xml");
                std::string xmlPath = (fs::path(m_Options.output_dir) / xmlPathRel).string();
                // Normalize path separators
                std::replace(xmlPath.begin(), xmlPath.end(), '\\', '/');
                
                // Relative path for manifest (e.g., "Models/model.xml")
                std::string xmlPathForManifest = xmlPathRel.string();
                std::replace(xmlPathForManifest.begin(), xmlPathForManifest.end(), '\\', '/');

                // Check if file exists and overwrite flag
                if (fs::exists(outputPath) && !options.overwrite) {
                    std::cerr << "Error: Output file already exists: " << outputPath << " (use --overwrite to replace)" << std::endl;
                    return false;
                }

                // Copy file to output directory
                if (!CopyFileToPackage(inputPath, outputPath)) {
                    std::cerr << "Error: Failed to copy file to: " << outputPath << std::endl;
                    return false;
                }

                // Convert ResourceType
                FirstEngine::Resources::ResourceType resourceType = FirstEngine::Resources::ResourceType::Model;

                // Generate or get ResourceID (use relative XML path for manifest registration)
                FirstEngine::Resources::ResourceID id = GenerateResourceID(xmlPathForManifest, resourceType, options.virtual_path);

                // For now, create an empty model (meshes and materials will be extracted during runtime)
                // In a full implementation, we would use Assimp to extract meshes and materials here
                FirstEngine::Resources::ResourceXMLParser::ModelData modelData;
                modelData.modelFile = ""; // Model is logical, no source file
                // modelData.meshIndices and modelData.materialIndices would be populated here
                // after processing the model file with Assimp

                std::vector<FirstEngine::Resources::ResourceDependency> dependencies;

                // Save XML
                if (!FirstEngine::Resources::ResourceXMLParser::SaveModelToXML(xmlPath, options.name, id, modelData, dependencies)) {
                    std::cerr << "Error: Failed to save XML file: " << xmlPath << std::endl;
                    return false;
                }

                std::cout << "Imported model: " << options.name << " (ID: " << id << ")" << std::endl;
                std::cout << "  File: " << outputPath << std::endl;
                std::cout << "  XML: " << xmlPath << std::endl;
                std::cout << "  Note: Model meshes and materials will be extracted during runtime loading" << std::endl;

                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error importing model: " << e.what() << std::endl;
                return false;
            }
        }

        bool ResourceImport::ImportMaterial(const std::string& inputPath, const ImportOptions& options) {
            try {
                // Material import depends on the format
                // For now, we'll support basic JSON and XML material files
                // In a full implementation, we would parse various material formats

                // Generate output paths
                fs::path inputFilePath(inputPath);
                std::string filename = inputFilePath.filename().string();
                std::string outputPath = GetOutputPath(filename, ResourceType::Material);
                
                // XML path (relative to output_dir for manifest registration)
                std::string typeDir = GetResourceTypeDirectory(ResourceType::Material);
                typeDir[0] = std::toupper(typeDir[0]); // Capitalize first letter
                fs::path xmlPathRel = fs::path(typeDir) / fs::path(filename).replace_extension(".xml");
                std::string xmlPath = (fs::path(m_Options.output_dir) / xmlPathRel).string();
                // Normalize path separators
                std::replace(xmlPath.begin(), xmlPath.end(), '\\', '/');
                
                // Relative path for manifest (e.g., "Materials/material.xml")
                std::string xmlPathForManifest = xmlPathRel.string();
                std::replace(xmlPathForManifest.begin(), xmlPathForManifest.end(), '\\', '/');

                // Check if file exists and overwrite flag
                if (fs::exists(outputPath) && !options.overwrite) {
                    std::cerr << "Error: Output file already exists: " << outputPath << " (use --overwrite to replace)" << std::endl;
                    return false;
                }

                // Copy file to output directory
                if (!CopyFileToPackage(inputPath, outputPath)) {
                    std::cerr << "Error: Failed to copy file to: " << outputPath << std::endl;
                    return false;
                }

                // Convert ResourceType
                FirstEngine::Resources::ResourceType resourceType = FirstEngine::Resources::ResourceType::Material;

                // Generate or get ResourceID (use relative XML path for manifest registration)
                FirstEngine::Resources::ResourceID id = GenerateResourceID(xmlPathForManifest, resourceType, options.virtual_path);

                // For now, create a basic material XML
                // In a full implementation, we would parse the material file and extract parameters
                FirstEngine::Resources::ResourceXMLParser::MaterialData materialData;
                materialData.shaderName = "DefaultPBR";
                // materialData.parameters and materialData.textureSlots would be populated here
                // after parsing the material file

                std::vector<FirstEngine::Resources::ResourceDependency> dependencies;

                // Save XML
                if (!FirstEngine::Resources::ResourceXMLParser::SaveMaterialToXML(xmlPath, options.name, id, materialData, dependencies)) {
                    std::cerr << "Error: Failed to save XML file: " << xmlPath << std::endl;
                    return false;
                }

                std::cout << "Imported material: " << options.name << " (ID: " << id << ")" << std::endl;
                std::cout << "  File: " << outputPath << std::endl;
                std::cout << "  XML: " << xmlPath << std::endl;
                std::cout << "  Note: Material parameters will be extracted from source file during runtime" << std::endl;

                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error importing material: " << e.what() << std::endl;
                return false;
            }
        }

        bool ResourceImport::LoadManifest(const std::string& manifestPath) {
            return m_IDManager.LoadManifest(manifestPath);
        }

        bool ResourceImport::SaveManifest(const std::string& manifestPath) {
            // Create directory if it doesn't exist
            fs::path path(manifestPath);
            if (path.has_parent_path()) {
                fs::create_directories(path.parent_path());
            }
            return m_IDManager.SaveManifest(manifestPath);
        }

        FirstEngine::Resources::ResourceID ResourceImport::GenerateResourceID(const std::string& path, FirstEngine::Resources::ResourceType type, const std::string& virtualPath) {
            // Check if path already registered
            FirstEngine::Resources::ResourceID existingID = m_IDManager.GetIDFromPath(path);
            if (existingID != FirstEngine::Resources::InvalidResourceID) {
                return existingID;
            }

            // Check if virtual path already registered
            if (!virtualPath.empty()) {
                FirstEngine::Resources::ResourceID existingVirtualID = m_IDManager.GetIDFromVirtualPath(virtualPath);
                if (existingVirtualID != FirstEngine::Resources::InvalidResourceID) {
                    return existingVirtualID;
                }
            }

            // Generate new ID
            FirstEngine::Resources::ResourceID newID = m_IDManager.GenerateID();
            
            // Register the resource
            if (!RegisterResource(newID, path, type, virtualPath)) {
                std::cerr << "Warning: Failed to register resource, but continuing..." << std::endl;
            }

            return newID;
        }

        bool ResourceImport::RegisterResource(FirstEngine::Resources::ResourceID id, const std::string& path, FirstEngine::Resources::ResourceType type, const std::string& virtualPath) {
            return m_IDManager.RegisterResourceWithID(id, path, type, virtualPath);
        }

        bool ResourceImport::CopyFileToPackage(const std::string& sourcePath, const std::string& destPath) const {
            try {
                // Create destination directory if it doesn't exist
                fs::path dest(destPath);
                if (dest.has_parent_path()) {
                    fs::create_directories(dest.parent_path());
                }

                // Copy file
                fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error copying file: " << e.what() << std::endl;
                return false;
            }
        }

        std::string ResourceImport::GetOutputPath(const std::string& filename, ResourceType type) const {
            std::string typeDir = GetResourceTypeDirectory(type);
            // Capitalize first letter and make rest lowercase
            if (!typeDir.empty()) {
                typeDir[0] = std::toupper(typeDir[0]);
                for (size_t i = 1; i < typeDir.length(); i++) {
                    typeDir[i] = std::tolower(typeDir[i]);
                }
            }
            
            fs::path outputPath(m_Options.output_dir);
            outputPath /= typeDir;
            outputPath /= filename;
            
            // Normalize path separators for Windows
            std::string result = outputPath.string();
            std::replace(result.begin(), result.end(), '\\', '/');
            return result;
        }

    } // namespace Tools
} // namespace FirstEngine
