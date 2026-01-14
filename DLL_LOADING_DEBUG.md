# Windows DLL 加载机制详解

## LoadLibraryA("vulkan-1.dll") 的搜索顺序

当调用 `LoadLibraryA("vulkan-1.dll")` 时，Windows 按以下顺序搜索 DLL：

### 1. 可执行文件所在目录
```
G:\AIHUMAN\WorkSpace\FirstEngine\build\bin\Debug\vulkan-1.dll
G:\AIHUMAN\WorkSpace\FirstEngine\build\bin\Release\vulkan-1.dll
```

### 2. 系统目录
```
C:\Windows\System32\vulkan-1.dll
C:\Windows\SysWOW64\vulkan-1.dll  (32位程序在64位系统上)
```

### 3. 16位系统目录（已废弃）
```
C:\Windows\System\vulkan-1.dll
```

### 4. Windows 目录
```
C:\Windows\vulkan-1.dll
```

### 5. 当前工作目录（Current Working Directory）
```
当前 PowerShell/CMD 的工作目录
```

### 6. PATH 环境变量中的目录
按顺序搜索 PATH 中的每个目录：
```
C:\VulkanSDK\1.3.xxx\Bin\vulkan-1.dll
C:\Program Files\Vulkan Runtime\vulkan-1.dll
... (PATH 中的其他目录)
```

## 如何查看 DLL 实际加载位置

### 方法 1: 使用 Process Monitor (ProcMon)

1. 下载 Process Monitor: https://docs.microsoft.com/en-us/sysinternals/downloads/procmon
2. 运行 Process Monitor
3. 设置过滤器：
   - Process Name: `FirstEngine.exe`
   - Operation: `LoadImage`
   - Path: `contains vulkan-1.dll`
4. 运行你的程序
5. 查看 "Path" 列，显示 DLL 的实际加载路径

### 方法 2: 使用代码检查

可以在代码中添加检查：

```cpp
#ifdef _WIN32
#include <Windows.h>
#include <psapi.h>

void CheckVulkanDLLLocation() {
    HMODULE hModule = LoadLibraryA("vulkan-1.dll");
    if (hModule) {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hModule, path, MAX_PATH)) {
            std::cout << "vulkan-1.dll loaded from: " << path << std::endl;
        }
        FreeLibrary(hModule);
    } else {
        DWORD error = GetLastError();
        std::cerr << "Failed to load vulkan-1.dll. Error: " << error << std::endl;
        
        // 检查常见位置
        const char* commonPaths[] = {
            "C:\\VulkanSDK\\1.3.275.0\\Bin\\vulkan-1.dll",
            "C:\\VulkanSDK\\1.3.250.0\\Bin\\vulkan-1.dll",
            "C:\\Windows\\System32\\vulkan-1.dll",
            // 添加更多路径...
        };
        
        std::cout << "Checking common locations:" << std::endl;
        for (const char* path : commonPaths) {
            if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                std::cout << "  Found: " << path << std::endl;
            }
        }
    }
}
#endif
```

### 方法 3: 使用 PowerShell 检查

```powershell
# 检查 PATH 环境变量
$env:PATH -split ';' | Where-Object { Test-Path "$_\vulkan-1.dll" }

# 检查常见位置
$commonPaths = @(
    "C:\VulkanSDK\*\Bin\vulkan-1.dll",
    "C:\Windows\System32\vulkan-1.dll",
    "C:\Program Files\Vulkan Runtime\vulkan-1.dll"
)
Get-ChildItem -Path $commonPaths -ErrorAction SilentlyContinue | Select-Object FullName
```

### 方法 4: 使用 Dependency Walker 或 Dependencies

1. 下载 Dependencies: https://github.com/lucasg/Dependencies
2. 打开你的可执行文件
3. 查看 `vulkan-1.dll` 的加载路径

## GLFW 如何加载 vulkan-1.dll

GLFW 的加载逻辑（在 `third_party/glfw/src/vulkan.c` 中）：

```c
// GLFW 尝试加载 vulkan-1.dll
#if defined(_GLFW_WIN32)
    _glfw.vk.handle = _glfwPlatformLoadModule("vulkan-1.dll");
#endif

// 如果失败，返回 false
if (!_glfw.vk.handle) {
    return GLFW_FALSE;
}
```

GLFW 使用平台特定的 `_glfwPlatformLoadModule()`，在 Windows 上它调用 `LoadLibraryA()`。

## 调试 DLL 加载问题

### 1. 启用 DLL 加载日志

在注册表中启用 DLL 加载日志（需要管理员权限）：

```powershell
# 启用 DLL 加载日志
reg add "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\FirstEngine.exe" /v GlobalFlag /t REG_DWORD /d 0x200 /f

# 查看日志
Get-EventLog -LogName Application -Source "Microsoft-Windows-Kernel-General" | Where-Object { $_.Message -like "*vulkan-1.dll*" }
```

### 2. 使用 Visual Studio 调试

在 Visual Studio 中：
1. 设置断点在 `LoadLibraryA` 调用
2. 查看调用堆栈
3. 检查 DLL 搜索路径

### 3. 检查环境变量

```powershell
# 检查 VULKAN_SDK
echo $env:VULKAN_SDK

# 检查 PATH
$env:PATH -split ';' | Select-String -Pattern "Vulkan"

# 检查当前工作目录
Get-Location
```

## 常见问题

### Q: 为什么 LoadLibraryA 找不到 DLL，但程序能运行？

A: 可能的原因：
1. 程序使用静态链接的导入库（`vulkan-1.lib`），Windows 的 DLL 加载器在程序启动时已经找到了 DLL
2. DLL 在 PATH 中，但 LoadLibraryA 的搜索顺序不同
3. 工作目录不同

### Q: 如何强制从特定路径加载？

A: 使用完整路径：

```cpp
HMODULE hModule = LoadLibraryA("C:\\VulkanSDK\\1.3.xxx\\Bin\\vulkan-1.dll");
```

### Q: 如何查看已加载的 DLL？

A: 使用 Task Manager 或 Process Explorer：
1. 打开 Task Manager
2. 转到 "Details" 标签
3. 右键点击你的进程
4. 选择 "Properties" -> "DLLs" 标签

或使用 PowerShell：

```powershell
Get-Process FirstEngine | Select-Object -ExpandProperty Modules | Where-Object { $_.FileName -like "*vulkan*" }
```

## 最佳实践

1. **将 DLL 放在可执行文件目录**：
   - 最可靠的方式
   - 不依赖系统配置
   - 便于部署

2. **使用完整路径**（如果知道路径）：
   - 最明确的方式
   - 但不够灵活

3. **检查加载结果**：
   - 总是检查 `LoadLibrary` 的返回值
   - 使用 `GetLastError()` 获取错误信息

4. **提供清晰的错误信息**：
   - 告诉用户 DLL 在哪里查找
   - 提供解决方案
