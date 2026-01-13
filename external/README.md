# External Dependencies

此目录用于存放外部依赖，以便在新环境中快速部署，无需单独安装和配置。

## 目录结构

```
external/
├── vulkan/          # Vulkan SDK
│   ├── Include/
│   ├── Lib/
│   └── ...
├── python/          # Python 3.9
│   ├── include/
│   ├── libs/
│   ├── python.exe
│   └── ...
└── README.md        # 本文件
```

## 设置步骤

### 方法1: 手动复制（推荐）

1. **复制 Vulkan SDK**:
   ```powershell
   # 从Vulkan SDK安装目录复制到项目
   xcopy /E /I "C:\VulkanSDK\1.3.xxx" "external\vulkan"
   ```

2. **复制 Python 3.9**:
   ```powershell
   # 从Python安装目录复制到项目
   xcopy /E /I "D:\Python3.9" "external\python"
   ```

### 方法2: 使用设置脚本

运行 `setup_external_deps.ps1` 脚本，它会自动检测并复制依赖：

```powershell
.\setup_external_deps.ps1
```

## 注意事项

1. **文件大小**: Vulkan SDK (~500MB) 和 Python (~100MB) 都比较大，这些目录已添加到 `.gitignore`，不会被提交到Git仓库。

2. **版本要求**:
   - Vulkan SDK: 1.3.x 或更高版本
   - Python: 3.9 或更高版本

3. **路径**: 复制后，CMake会自动检测并使用 `external/` 目录中的依赖，无需设置环境变量。

4. **更新**: 如果需要更新依赖，只需替换 `external/` 目录中的相应文件夹即可。

## 验证

运行环境检查脚本验证设置：

```powershell
.\check_environment.ps1
```

如果看到本地路径被检测到，说明设置成功。
