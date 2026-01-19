# FirstEngine 快速部署指南

本指南帮助您在新环境中快速部署 FirstEngine 项目，无需手动安装和配置系统依赖。

## 方法1: 使用本地依赖（推荐）

### 步骤1: 克隆项目并初始化子模块

```powershell
git clone https://github.com/liangz0707/FirstEngine.git
cd FirstEngine
git submodule update --init --recursive
```

> 如果你在这一步遇到“网络问题”（超时、连接被重置、无法访问 GitHub），通常并不是引擎运行时联网，而是 **子模块拉取失败**。
>
> - Windows：多重试几次 `git submodule update --init --recursive`
> - Linux/macOS：可使用 `scripts/init_submodules.sh`（带指数退避重试）

### 步骤2: 设置本地依赖

运行设置脚本，自动复制 Vulkan SDK 和 Python 到项目目录：

```powershell
.\setup_external_deps.ps1
```

脚本会自动：
- 检测系统中的 Vulkan SDK 和 Python 安装位置
- 复制必要的文件到 `external/` 目录
- 配置项目使用本地依赖

### 步骤3: 验证环境

```powershell
.\check_environment.ps1
```

### 步骤4: 构建项目

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 方法2: 手动复制依赖

如果您已经有 Vulkan SDK 和 Python 的安装，可以手动复制：

### 复制 Vulkan SDK

```powershell
# 从Vulkan SDK安装目录复制
xcopy /E /I "C:\VulkanSDK\1.3.xxx" "external\vulkan"
```

### 复制 Python

```powershell
# 从Python安装目录复制
xcopy /E /I "D:\Python3.9" "external\python"
```

**注意**: 只需要复制以下目录和文件：

**Vulkan SDK**:
- `Include/` - 头文件
- `Lib/` - 库文件
- `Bin/` - 可执行文件（可选）
- `vulkan-1.dll`, `vulkan-1.lib` - 主要库文件

**Python**:
- `include/` - 头文件
- `libs/` - 库文件
- `DLLs/` - DLL文件
- `Lib/` - Python标准库（可选，如果不需要标准库可以省略）
- `python.exe`, `pythonw.exe` - 可执行文件
- `python39.dll`, `python3.dll` - 主要DLL文件

## 方法3: 使用系统安装的依赖

如果您不想复制依赖到项目，可以：

1. 安装 Vulkan SDK 并设置 `VULKAN_SDK` 环境变量
2. 安装 Python 3.9+ 并确保在 PATH 中，或设置 `PYTHON_ROOT_DIR` 环境变量
3. 直接构建项目

## 优势对比

| 方法 | 优点 | 缺点 |
|------|------|------|
| 本地依赖 | 快速部署，无需配置环境变量，版本固定 | 占用磁盘空间，需要复制文件 |
| 系统安装 | 不占用项目空间，可共享 | 需要配置环境变量，版本可能不一致 |

## 在新机器上部署

1. **克隆项目**:
   ```powershell
   git clone https://github.com/liangz0707/FirstEngine.git
   cd FirstEngine
   git submodule update --init --recursive
   ```

2. **复制依赖**（如果使用本地依赖）:
   - 从其他已配置的机器复制 `external/` 目录
   - 或运行 `setup_external_deps.ps1` 脚本

3. **构建**:
   ```powershell
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

## 注意事项

1. **文件大小**: 
   - Vulkan SDK: ~500MB
   - Python: ~100MB
   - 总计: ~600MB

2. **Git忽略**: `external/` 目录已添加到 `.gitignore`，不会被提交到Git仓库。

3. **版本要求**:
   - Vulkan SDK: 1.3.x 或更高
   - Python: 3.9 或更高

4. **跨平台**: 本地依赖方法主要适用于 Windows。Linux/macOS 用户建议使用系统包管理器安装依赖。

## 故障排除

### CMake找不到依赖

1. 检查 `external/` 目录是否存在
2. 检查 `external/vulkan/` 和 `external/python/` 目录结构
3. 运行 `check_environment.ps1` 检查

### 编译错误

1. 确保依赖文件完整（特别是头文件和库文件）
2. 检查 CMake 输出，确认使用了本地路径
3. 查看 `ENVIRONMENT.md` 了解详细要求

## 相关文档

- `ENVIRONMENT.md` - 环境变量和系统依赖详细说明
- `external/README.md` - 本地依赖目录说明
- `check_environment.ps1` - 环境检查脚本
