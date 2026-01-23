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
#include <iostream>

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
                std::cerr << "TextureLoader::Load: Failed to get resolved path for texture ID " << id << std::endl;
                std::cerr << "  Make sure the texture is registered in resource_manifest.json" << std::endl;
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

            // Check if image file exists
            if (!fs::exists(imagePath)) {
                std::cerr << "TextureLoader::Load: Image file not found: " << imagePath << std::endl;
                std::cerr << "  Texture ID: " << id << ", XML file: " << xmlFilePath << std::endl;
                return result;
            }
            
            // Load image data using ImageLoader
            result.imageData = ImageLoader::LoadFromFile(imagePath);
            if (result.imageData.data.empty()) {
                std::cerr << "TextureLoader::Load: Failed to load image data from: " << imagePath << std::endl;
                std::cerr << "  Texture ID: " << id << std::endl;
                std::cerr << "  This may indicate an unsupported format or corrupted file" << std::endl;
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
            // Formats supported by stb_image (used by ImageLoader)
            // NOTE: DDS and TIFF are NOT supported by stb_image, but kept for backward compatibility
            // TODO: If DDS/TIFF support is needed, implement a specialized loader
            return { 
                ".xml",      // Texture resource XML file
                ".jpg",      // JPEG (baseline & progressive)
                ".jpeg",     // JPEG
                ".png",      // PNG (1/2/4/8/16-bit per channel)
                ".bmp",      // BMP (non-1bpp, non-RLE)
                ".tga",      // TGA (including 16-bit)
                ".psd",      // PSD (composited view, 8/16-bit)
                ".gif",      // GIF (static & animated)
                ".hdr",      // HDR (Radiance rgbE)
                ".pic",      // PIC (Softimage PIC)
                ".ppm",      // PNM PPM (binary)
                ".pgm",      // PNM PGM (binary)
                // ".dds",   // DDS - NOT supported by stb_image
                // ".tiff",  // TIFF - NOT supported by stb_image
            };
        }

    } // namespace Resources
} // namespace FirstEngine
