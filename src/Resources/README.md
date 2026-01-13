# FirstEngine_Resources Module

Enhanced resource loading module with support for multiple image and model formats.

## Features

### Image Loading
- **Supported Formats**: JPEG, PNG, BMP, TGA, DDS, TIFF, HDR
- **Library**: stb_image (header-only, lightweight)
- **Features**:
  - Load from file or memory
  - Automatic format detection
  - Support for various color channels (grayscale, RGB, RGBA)

### Model Loading
- **Supported Formats**: FBX, OBJ, DAE, 3DS, BLEND, and 40+ more formats
- **Library**: Assimp (Open Asset Import Library)
- **Features**:
  - Mesh loading with vertices, normals, texture coordinates
  - Material information
  - Bone/skeleton data
  - Multiple meshes per model

## Usage

### Image Loading

```cpp
#include "FirstEngine/Resources/ImageLoader.h"

using namespace FirstEngine::Resources;

// Load image from file
try {
    ImageData image = ImageLoader::LoadFromFile("texture.png");
    std::cout << "Loaded: " << image.width << "x" << image.height 
              << " (" << image.channels << " channels)" << std::endl;
    
    // Use image.data for pixel data
    // ...
    
    // Free when done
    ImageLoader::FreeImageData(image);
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Detect format
ImageFormat format = ImageLoader::DetectFormat("texture.jpg");
if (format == ImageFormat::JPEG) {
    // Handle JPEG
}
```

### Model Loading

```cpp
#include "FirstEngine/Resources/ModelLoader.h"

using namespace FirstEngine::Resources;

// Load model from file
try {
    Model model = ModelLoader::LoadFromFile("model.fbx");
    
    std::cout << "Loaded model: " << model.name << std::endl;
    std::cout << "Meshes: " << model.meshes.size() << std::endl;
    std::cout << "Bones: " << model.bones.size() << std::endl;
    
    // Process meshes
    for (const auto& mesh : model.meshes) {
        std::cout << "Mesh: " << mesh.vertices.size() << " vertices, "
                  << mesh.indices.size() << " indices" << std::endl;
        std::cout << "Material: " << mesh.materialName << std::endl;
    }
    
    // Process bones
    for (const auto& bone : model.bones) {
        std::cout << "Bone: " << bone.name << std::endl;
    }
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Check if format is supported
if (ModelLoader::IsFormatSupported("model.obj")) {
    // Format is supported
}

// Get all supported formats
auto formats = ModelLoader::GetSupportedFormats();
for (const auto& format : formats) {
    std::cout << "Supported: ." << format << std::endl;
}
```

## Dependencies

- **stb**: Single-file header-only library for image loading
- **assimp**: Open Asset Import Library for model loading
- **GLM**: Mathematics library (for vectors and matrices)

## Supported Formats

### Image Formats
- JPEG (.jpg, .jpeg)
- PNG (.png)
- BMP (.bmp)
- TGA (.tga)
- DDS (.dds)
- TIFF (.tiff, .tif)
- HDR (.hdr)

### Model Formats
- FBX (.fbx)
- OBJ (.obj)
- DAE/COLLADA (.dae)
- 3DS (.3ds)
- BLEND (.blend)
- And 40+ more formats supported by Assimp

See Assimp documentation for complete list of supported formats.
