# Vulkan SDK 1.3.275.0 结构说明

## 重要发现

**新版本的 Vulkan SDK (1.3.275.0) 不再包含 `vulkan-1.dll`！**

这是正常的设计变更。新版本的 Vulkan SDK 将运行时组件和开发工具分离了。

## Vulkan SDK 目录结构

### Vulkan SDK 1.3.275.0 包含：

```
D:\VulkanSDK\1.3.275.0\
├── Bin\              # 开发工具和验证层 DLL
│   ├── dxcompiler.dll
│   ├── shaderc_shared.dll
│   ├── VkLayer_*.dll (验证层)
│   └── ... (但不包含 vulkan-1.dll)
├── Include\          # 头文件
├── Lib\              # 库文件 (vulkan-1.lib)
└── Config\           # 配置文件
```

### 不包含：

- ❌ `vulkan-1.dll` - 运行时加载器
- ❌ `vulkan-1.lib` - 可能也不在根目录（在 Lib 目录中）

## vulkan-1.dll 的位置

`vulkan-1.dll` 现在应该从以下位置获取：

### 1. 系统目录（推荐）

```
C:\Windows\System32\vulkan-1.dll
```

这个文件通常由以下方式安装：
- **显卡驱动**（NVIDIA/AMD/Intel 驱动通常包含）
- **Vulkan Runtime 安装程序**（单独安装）

### 2. Vulkan Runtime 安装程序

从 LunarG 下载并安装 Vulkan Runtime：
- 下载地址：https://vulkan.lunarg.com/sdk/home#runtime
- 选择 "Vulkan Runtime" 而不是 "Vulkan SDK"
- 安装后，DLL 会在 `C:\Windows\System32\` 中

## 解决方案

### 方案 1: 使用系统 DLL（如果已安装）

如果系统中有 `vulkan-1.dll`（在 `C:\Windows\System32\`），可以：

1. **直接使用**（如果 DLL 在 PATH 中）
2. **复制到项目**：
   ```powershell
   Copy-Item "C:\Windows\System32\vulkan-1.dll" "external\vulkan\"
   ```

### 方案 2: 安装 Vulkan Runtime

1. 下载 Vulkan Runtime：
   - 地址：https://vulkan.lunarg.com/sdk/home#runtime
   - 或从显卡厂商获取

2. 安装后，DLL 会在系统目录中

### 方案 3: 从旧版本 SDK 获取

如果你有旧版本的 Vulkan SDK（如 1.3.224.1），可以从那里复制：

```powershell
# 检查旧版本
$oldSdk = "C:\VulkanSDK\1.3.224.1"
if (Test-Path "$oldSdk\Bin\vulkan-1.dll") {
    Copy-Item "$oldSdk\Bin\vulkan-1.dll" "external\vulkan\"
}
```

### 方案 4: 从显卡驱动获取

NVIDIA/AMD/Intel 的显卡驱动通常包含 Vulkan Runtime。更新驱动后，DLL 会在系统目录中。

## 验证 vulkan-1.dll

运行检查脚本：

```powershell
.\CHECK_VULKAN_DLL.ps1 -DllPath "C:\Windows\System32\vulkan-1.dll"
```

## 为什么新版本 SDK 不包含 DLL？

这是 Vulkan SDK 的设计决策：

1. **分离关注点**：
   - SDK = 开发工具（编译器、验证层、头文件）
   - Runtime = 运行时组件（vulkan-1.dll）

2. **减少 SDK 大小**：SDK 不需要包含运行时 DLL

3. **版本管理**：运行时可以独立更新，不需要重新下载整个 SDK

## 推荐做法

1. **开发环境**：
   - 安装完整的 Vulkan SDK（用于编译）
   - 安装 Vulkan Runtime（用于运行）

2. **部署**：
   - 将 `vulkan-1.dll` 复制到可执行文件目录
   - 或确保用户在系统 PATH 中有 DLL

3. **项目配置**：
   - 使用 CMakeLists.txt 自动复制 DLL 到输出目录
   - 或从系统目录复制到 `external/vulkan`

## 快速修复

如果系统中有 `vulkan-1.dll`，快速复制到项目：

```powershell
# 检查系统 DLL
if (Test-Path "C:\Windows\System32\vulkan-1.dll") {
    # 确保目录存在
    New-Item -ItemType Directory -Force -Path "external\vulkan" | Out-Null
    
    # 复制 DLL
    Copy-Item "C:\Windows\System32\vulkan-1.dll" "external\vulkan\" -Force
    Write-Host "已复制 vulkan-1.dll 到 external\vulkan\" -ForegroundColor Green
} else {
    Write-Host "系统目录中未找到 vulkan-1.dll" -ForegroundColor Red
    Write-Host "请安装 Vulkan Runtime 或更新显卡驱动" -ForegroundColor Yellow
}
```
