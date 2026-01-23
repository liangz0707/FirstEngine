# ImageLoader 支持的文件格式

## stb_image 支持的文件类型

`ImageLoader::LoadFromFile` 使用 `stb_image` 库加载图像，支持以下文件格式：

### ✅ 完全支持

1. **JPEG** (.jpg, .jpeg)
   - Baseline 和 Progressive JPEG
   - 不支持 12 bpc/arithmetic（与标准 IJG 库相同）

2. **PNG** (.png)
   - 1/2/4/8/16 位每通道
   - 自动去调色板（Paletted PNG）

3. **BMP** (.bmp)
   - 非 1bpp，非 RLE 压缩
   - 支持 1-bit BMP（从 v2.17 开始）

4. **TGA** (.tga)
   - 支持 16 位 TGA（从 v2.09 开始）

5. **PSD** (.psd)
   - 仅合成视图（composited view）
   - 无额外通道
   - 8/16 位每通道

6. **GIF** (.gif)
   - 静态 GIF
   - 动画 GIF（需要额外 API）
   - *comp 始终报告为 4 通道（RGBA）

7. **HDR** (.hdr)
   - Radiance rgbE 格式
   - 高动态范围图像

8. **PIC** (.pic)
   - Softimage PIC 格式

9. **PNM** (.ppm, .pgm)
   - PPM（Portable Pixmap）和 PGM（Portable Graymap）
   - 仅二进制格式
   - 支持 16 位 PNM（从 v2.27 开始）

### ❌ 不支持

- **DDS** (.dds) - DirectDraw Surface，stb_image 不支持
- **TIFF** (.tiff, .tif) - Tagged Image File Format，stb_image 不支持
- **WebP** (.webp) - Google WebP，stb_image 不支持
- **EXR** (.exr) - OpenEXR，stb_image 不支持

### 当前实现状态

**ImageLoader::DetectFormat** 方法中检测了以下格式：
- ✅ JPEG
- ✅ PNG
- ✅ BMP
- ✅ TGA
- ✅ DDS（检测但 stb_image 不支持加载）
- ✅ TIFF（检测但 stb_image 不支持加载）
- ✅ HDR

**TextureLoader::GetSupportedFormats** 返回：
```cpp
{ ".xml", ".jpg", ".jpeg", ".png", ".bmp", ".tga", ".dds", ".tiff", ".hdr" }
```

**注意**：虽然 `.dds` 和 `.tiff` 在列表中，但 `stb_image` 实际上不支持这些格式。如果尝试加载这些格式，`stbi_load` 会失败。

### 建议

1. **更新 TextureLoader::GetSupportedFormats**：移除 `.dds` 和 `.tiff`，或添加注释说明需要其他加载器
2. **添加 PSD、GIF、PIC、PNM 支持**：这些格式 stb_image 支持，但当前未在列表中
3. **考虑添加 DDS 和 TIFF 支持**：如果需要这些格式，可以使用其他库（如 DirectXTex 用于 DDS，libtiff 用于 TIFF）

### 实际可用的格式

基于 stb_image 的实际支持，以下格式可以成功加载：
- `.jpg`, `.jpeg` - JPEG
- `.png` - PNG
- `.bmp` - BMP
- `.tga` - TGA
- `.psd` - PSD
- `.gif` - GIF
- `.hdr` - HDR
- `.pic` - PIC
- `.ppm`, `.pgm` - PNM
