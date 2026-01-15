#include "FirstEngine/Resources/TextureLoader.h"
#include "FirstEngine/Resources/ResourceXMLParser.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        TextureLoader::LoadResult TextureLoader::Load(ResourceID id) {
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
            if (cached.texture) {
                // Return cached data
                ITexture* texture = cached.texture;
                result.metadata = texture->GetMetadata();
                result.imageData.width = texture->GetWidth();
                result.imageData.height = texture->GetHeight();
                result.imageData.channels = texture->GetChannels();
                result.imageData.hasAlpha = texture->HasAlpha();
                
                // Copy image data
                size_t dataSize = texture->GetDataSize();
                result.imageData.data.resize(dataSize);
                std::memcpy(result.imageData.data.data(), texture->GetData(), dataSize);
                
                result.success = true;
                return result;
            }

            // Resolve XML file path
            std::string xmlFilePath = resolvedPath;
            std::string ext = fs::path(xmlFilePath).extension().string();
            if (ext != ".xml" && ext != ".tex") {
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

            // Get texture data from XML
            ResourceXMLParser::TextureData textureData;
            if (!parser.GetTextureData(textureData)) {
                return result;
            }

            // Resolve image file path relative to XML file directory
            std::string xmlDir = fs::path(xmlFilePath).parent_path().string();
            std::string imagePath = textureData.imageFile;
            
            // If image path is relative, resolve it relative to XML file
            if (!fs::path(imagePath).is_absolute()) {
                imagePath = (fs::path(xmlDir) / imagePath).string();
            }

            // Load image data using ImageLoader
            result.imageData = ImageLoader::LoadFromFile(imagePath);
            if (result.imageData.data.empty()) {
                return result;
            }

            // Verify dimensions match XML (if specified)
            if (textureData.width > 0 && textureData.height > 0) {
                if (result.imageData.width != textureData.width || result.imageData.height != textureData.height) {
                    ImageLoader::FreeImageData(result.imageData);
                    return result; // Dimension mismatch
                }
            }

            result.metadata.isLoaded = true;
            result.success = true;
            return result;
        }

        bool TextureLoader::Save(const std::string& xmlFilePath,
                                       const std::string& name,
                                       ResourceID id,
                                       const std::string& imageFilePath,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t channels,
                                       bool hasAlpha) {
            // Create relative path from XML file to image file
            std::string xmlDir = fs::path(xmlFilePath).parent_path().string();
            std::string imageDir = fs::path(imageFilePath).parent_path().string();
            
            std::string relativeImagePath = imageFilePath;
            if (fs::path(imageFilePath).is_absolute() && !xmlDir.empty()) {
                // Try to make relative path
                fs::path xmlPath(xmlDir);
                fs::path imagePath(imageFilePath);
                
                // Check if they share a common root
                auto xmlIt = xmlPath.begin();
                auto imgIt = imagePath.begin();
                
                // Find common root
                while (xmlIt != xmlPath.end() && imgIt != imagePath.end() && *xmlIt == *imgIt) {
                    ++xmlIt;
                    ++imgIt;
                }
                
                if (xmlIt == xmlPath.begin()) {
                    // No common root, use absolute path
                    relativeImagePath = imageFilePath;
                } else {
                    // Build relative path
                    relativeImagePath = "";
                    for (; imgIt != imagePath.end(); ++imgIt) {
                        if (!relativeImagePath.empty()) {
                            relativeImagePath += "/";
                        }
                        relativeImagePath += imgIt->string();
                    }
                }
            }

            ResourceXMLParser::TextureData textureData;
            textureData.imageFile = relativeImagePath;
            textureData.width = width;
            textureData.height = height;
            textureData.channels = channels;
            textureData.hasAlpha = hasAlpha;

            return ResourceXMLParser::SaveTextureToXML(xmlFilePath, name, id, textureData);
        }

        bool TextureLoader::IsFormatSupported(const std::string& filepath) {
            std::string ext = fs::path(filepath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return ext == ".xml" || ImageLoader::DetectFormat(filepath) != ImageFormat::Unknown;
        }

        std::vector<std::string> TextureLoader::GetSupportedFormats() {
            return { ".xml", ".jpg", ".jpeg", ".png", ".bmp", ".tga", ".dds", ".tiff", ".hdr" };
        }

    } // namespace Resources
} // namespace FirstEngine
