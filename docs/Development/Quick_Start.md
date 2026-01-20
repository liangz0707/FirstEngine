# FirstEngine 快速开始指南

## 编译和运行

### 1. 编译项目

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 2. 运行程序

```bash
# Windows
.\bin\Release\FirstEngine.exe

# Linux/Mac
./bin/FirstEngine
```

### 3. 命令行选项

```bash
# 指定窗口大小
.\FirstEngine.exe --width 1920 --height 1080

# 指定配置文件
.\FirstEngine.exe --config configs/my_pipeline.pipeline

# 显示帮助
.\FirstEngine.exe --help
```

## 配置文件

程序会自动在 `configs/` 目录下查找 `deferred_rendering.pipeline` 配置文件。如果不存在，会自动创建默认的延迟渲染配置。

### 配置文件位置

- 默认: `configs/deferred_rendering.pipeline`
- 自定义: 使用 `--config` 参数指定

### 配置文件格式

```
pipeline {
  name "PipelineName"
  width 1280
  height 720
  resource {
    name "ResourceName"
    type "attachment"
    width 1280
    height 720
    format "R8G8B8A8_UNORM"
  }
  pass {
    name "PassName"
    type "geometry"
    write "ResourceName"
  }
}
```

## RenderDoc 使用

### 方法 1: 使用 RenderDoc 启动器

1. 打开 RenderDoc
2. 点击 "Launch Application"
3. 选择 `FirstEngine.exe`
4. 点击 "Launch"

### 方法 2: 注入到进程

1. 启动 FirstEngine
2. 在 RenderDoc 中选择 "Inject into Process"
3. 选择 FirstEngine 进程

### 验证 RenderDoc 集成

如果 RenderDoc 成功初始化，程序启动时会显示：
```
RenderDoc API initialized!
```

## 延迟渲染管线

默认的延迟渲染管线包含：

1. **Geometry Pass**: 渲染几何体到 G-Buffer
   - GBufferAlbedo (反照率)
   - GBufferNormal (法线)
   - GBufferDepth (深度)

2. **Lighting Pass**: 使用 G-Buffer 进行光照计算
   - 输出到 FinalOutput

## 故障排除

### 问题: 程序无法启动

- 检查 Vulkan SDK 是否已安装
- 检查显卡驱动是否支持 Vulkan
- 查看控制台错误信息

### 问题: RenderDoc 无法工作

- 确保已安装 RenderDoc
- 检查 `renderdoc.dll` 是否在系统路径中
- 尝试使用 RenderDoc 启动器启动程序

### 问题: 配置文件无法加载

- 检查 `configs/` 目录是否存在
- 检查配置文件格式是否正确
- 查看控制台错误信息

## 下一步

- 查看 `ARCHITECTURE_RHI.md` 了解架构设计
- 查看 `COMPLETION_SUMMARY.md` 了解完整功能列表
- 查看 `README_RENDERDOC.md` 了解 RenderDoc 详细使用
