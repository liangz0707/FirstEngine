# 手动下载测试纹理图片指南

由于自动下载可能受到网络限制，这里提供手动下载和替换图片的方法。

## stb_image 支持的图片格式

- JPEG (.jpg, .jpeg)
- PNG (.png)
- BMP (.bmp)
- TGA (.tga)
- PSD (.psd)
- GIF (.gif)
- HDR (.hdr)
- PIC (.pic)
- PNM (.ppm, .pgm)

## 推荐的测试图片资源

### 1. 免费纹理资源网站

- **Poly Haven** (https://polyhaven.com/textures)
  - 高质量 PBR 纹理
  - 支持 PNG 格式
  - 包括 Albedo, Normal, Roughness, Metallic 等贴图

- **Texture Haven** (https://texturehaven.com/)
  - 免费高质量纹理
  - PNG 格式

- **CC0 Textures** (https://cc0textures.com/)
  - CC0 许可的纹理
  - 包含完整的 PBR 贴图集

- **OpenGameArt** (https://opengameart.org/)
  - 游戏美术资源
  - 多种格式

### 2. 测试图片下载建议

对于 `build/Package/Textures/` 目录中的文件，建议下载以下类型的图片：

1. **example_texture.png** - 通用测试纹理（512x512 或 1024x1024）
2. **albedo_texture.png** - 基础颜色贴图（漫反射贴图）
3. **normal_texture.png** - 法线贴图（通常为蓝紫色调）
4. **metallic_roughness_texture.png** - 金属度/粗糙度贴图
5. **ao_texture.png** - 环境光遮蔽贴图（灰度图）
6. **lightmap.png** - 光照贴图
7. **shadow_texture.png** - 阴影贴图（灰度图）
8. **hdr_texture.png** - HDR 环境贴图（注意：HDR 格式应为 .hdr，不是 .png）

### 3. 快速测试图片生成

如果只需要快速测试，可以使用以下在线工具生成测试图片：

- **Placeholder.com** (https://via.placeholder.com/)
  - 示例：https://via.placeholder.com/512x512.png
  - 可以自定义尺寸和颜色

- **DummyImage** (https://dummyimage.com/)
  - 生成占位图片

### 4. 替换步骤

1. 下载图片文件
2. 将图片文件复制到 `build/Package/Textures/` 目录
3. 确保文件名与 XML 文件中引用的文件名一致
4. 检查 XML 文件中的尺寸信息是否匹配（可选）

### 5. 检查 XML 文件

每个纹理都有一个对应的 XML 文件，例如 `example_texture.xml`。确保：

- XML 文件中的 `<ImageFile>` 节点指向正确的图片文件名
- 如果 XML 中指定了宽度和高度，确保与实际图片尺寸匹配（或删除这些限制）

### 6. 推荐的测试图片尺寸

- 小纹理：256x256
- 中等纹理：512x512
- 大纹理：1024x1024
- 超大纹理：2048x2048

建议使用 512x512 或 1024x1024 进行测试。

## 注意事项

1. **文件格式**：确保下载的图片是 stb_image 支持的格式（主要是 PNG、JPEG）
2. **文件大小**：避免使用过大的图片文件（>10MB），可能影响加载速度
3. **命名规范**：保持文件名与 XML 文件中的引用一致
4. **备份**：替换前建议备份原始文件
