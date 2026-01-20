# RenderDoc 集成说明

## 概述

FirstEngine 已集成 RenderDoc 支持，可以在运行时进行截帧调试。

## 使用方法

### 1. 安装 RenderDoc

从 [RenderDoc 官网](https://renderdoc.org/) 下载并安装 RenderDoc。

### 2. 启动程序

正常启动 FirstEngine 程序。如果检测到 RenderDoc DLL，程序会自动初始化 RenderDoc API。

### 3. 截帧

程序会在每一帧自动调用 `StartFrameCapture` 和 `EndFrameCapture`。你可以：

- **方法 1**: 使用 RenderDoc 的启动器启动程序
  - 打开 RenderDoc
  - 点击 "Launch Application"
  - 选择 FirstEngine 可执行文件
  - 点击 "Launch"

- **方法 2**: 使用 RenderDoc 的注入功能
  - 启动程序后，在 RenderDoc 中选择 "Inject into Process"
  - 选择 FirstEngine 进程
  - 程序会自动开始捕获帧

### 4. 查看捕获的帧

在 RenderDoc 中，你可以：
- 查看每一帧的渲染命令
- 检查纹理和缓冲区内容
- 调试着色器
- 分析渲染性能

## 代码集成

RenderDoc 集成代码位于 `src/Application/RenderApp.cpp`：

```cpp
void InitializeRenderDoc() {
    // 检测并初始化 RenderDoc API
}

void BeginRenderDocFrame() {
    // 开始帧捕获
    m_RenderDocAPI->StartFrameCapture(nullptr, nullptr);
}

void EndRenderDocFrame() {
    // 结束帧捕获
    m_RenderDocAPI->EndFrameCapture(nullptr, nullptr);
}
```

## 注意事项

1. RenderDoc 只在 Windows 平台上集成
2. 需要安装 RenderDoc 并确保 `renderdoc.dll` 在系统路径中
3. 如果未检测到 RenderDoc，程序会正常运行，只是不会进行截帧
4. 每一帧都会进行捕获，可能会影响性能

## 故障排除

如果 RenderDoc 无法工作：

1. 确保已安装 RenderDoc
2. 检查 `renderdoc.dll` 是否在系统路径中
3. 尝试使用 RenderDoc 的启动器启动程序
4. 检查程序输出，看是否有 "RenderDoc API initialized!" 消息
