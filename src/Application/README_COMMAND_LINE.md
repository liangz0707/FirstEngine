# FirstEngine 命令行参数说明

FirstEngine 支持通过命令行参数配置和运行应用程序。

## 基本用法

```bash
FirstEngine.exe [OPTIONS] [ARGS...]
```

## 可用选项

### 窗口选项

- `--width`, `-w <int>` - 窗口宽度（默认: 1280）
- `--height`, `-h <int>` - 窗口高度（默认: 720）
- `--title`, `-t <string>` - 窗口标题（默认: "FirstEngine"）

### 运行模式

- `--headless` - 无头模式运行（不创建窗口）
- `--config`, `-c <string>` - 配置文件路径
- `--log-level`, `-l <string>` - 日志级别（debug, info, warning, error，默认: info）

### 信息选项

- `--help`, `-?` - 显示帮助信息
- `--version`, `-v` - 显示版本信息

## 使用示例

### 基本运行

```bash
# 使用默认设置
FirstEngine.exe

# 指定窗口大小
FirstEngine.exe --width 1920 --height 1080

# 指定窗口标题
FirstEngine.exe --title "My Game"
```

### 无头模式

```bash
# 无头模式运行（不创建窗口，适合服务器或批处理）
FirstEngine.exe --headless
```

### 配置文件

```bash
# 指定配置文件
FirstEngine.exe --config config.json
```

### 日志级别

```bash
# 设置日志级别为debug
FirstEngine.exe --log-level debug

# 设置日志级别为error
FirstEngine.exe --log-level error
```

### 组合使用

```bash
# 组合多个选项
FirstEngine.exe --width 1920 --height 1080 --title "My Game" --log-level debug

# 使用短选项
FirstEngine.exe -w 1920 -h 1080 -t "My Game" -l debug
```

### 位置参数

```bash
# 位置参数（非选项参数）
FirstEngine.exe scene.json model.obj texture.png
```

### 帮助和版本

```bash
# 显示帮助信息
FirstEngine.exe --help
FirstEngine.exe -?

# 显示版本信息
FirstEngine.exe --version
FirstEngine.exe -v
```

## 在代码中使用

```cpp
#include "FirstEngine/Core/CommandLine.h"

using namespace FirstEngine::Core;

CommandLineParser parser;

// 添加参数定义
parser.AddArgument("width", "w", "Window width", ArgumentType::Integer, false, "1280");
parser.AddArgument("height", "h", "Window height", ArgumentType::Integer, false, "720");

// 解析命令行
if (!parser.Parse(argc, argv)) {
    std::cerr << "Error: " << parser.GetError() << std::endl;
    return 1;
}

// 获取参数值
int width = parser.GetInt("width", 1280);
int height = parser.GetInt("height", 720);
bool headless = parser.GetBool("headless", false);
std::string title = parser.GetString("title", "FirstEngine");

// 获取位置参数
auto positionalArgs = parser.GetPositionalArgs();
```

## 参数类型

- **Flag** - 布尔标志（存在为true，不存在为false）
- **String** - 字符串值
- **Integer** - 整数值
- **Float** - 浮点数值
- **List** - 列表值（可多次出现）

## 注意事项

1. 选项可以以 `-` 或 `--` 开头
2. 短选项使用单个 `-`，长选项使用 `--`
3. 必需参数如果未提供会报错
4. 位置参数（非选项参数）可以通过 `GetPositionalArgs()` 获取
5. 使用 `--help` 或 `-?` 可以查看所有可用选项
