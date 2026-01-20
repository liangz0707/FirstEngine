# 修复调试器退出问题

## 问题

使用 `FirstEngine --editor` 启动时，在执行 `CreateProcessA` 后，Visual Studio 调试器会退出调试模式。

## 原因

当 `CreateProcessA` 成功创建子进程（FirstEngineEditor）后，代码立即执行 `return 0`，导致主进程（FirstEngine.exe）退出。Visual Studio 调试器附加到主进程，主进程退出后调试器也就退出了。

## 已完成的修复

我已经修改了 `RenderApp.cpp`，在创建子进程后：
1. **等待子进程结束**：使用 `WaitForSingleObject` 等待编辑器进程退出
2. **获取退出码**：获取子进程的退出码并返回
3. **保持主进程存活**：这样调试器可以继续附加到主进程

## 修改内容

```cpp
if (CreateProcessA(...)) {
    CloseHandle(pi.hThread);
    
    // 等待编辑器进程退出
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // 获取退出码
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    
    return static_cast<int>(exitCode);
}
```

## 现在的行为

1. **FirstEngine.exe** 启动
2. 创建 **FirstEngineEditor.exe** 子进程
3. **FirstEngine.exe** 等待编辑器进程结束
4. 调试器保持附加到 **FirstEngine.exe**
5. 编辑器退出后，**FirstEngine.exe** 也退出

## 调试 C++ 代码

现在可以正常调试 C++ 代码了：

1. **设置断点**（在 `RenderApp.cpp` 的 `main` 函数或 `CreateProcessA` 调用前后）

2. **启动调试**：
   - 配置：**Debug x64**
   - 启动项目：**FirstEngine**
   - 命令行参数：`--editor`（在项目属性中设置）

3. **按 F5 启动调试**

4. **断点应该可以正常工作**：
   - 在 `CreateProcessA` 调用前设置断点
   - 在 `WaitForSingleObject` 调用前设置断点
   - 调试器不会退出

## 设置命令行参数

在 Visual Studio 中设置命令行参数：

1. **右键 `FirstEngine` 项目 → 属性**

2. **配置属性 → 调试 → 命令参数**：
   - 输入：`--editor`

3. **或者配置属性 → 调试 → 命令**：
   - 可以留空（使用默认的可执行文件）

## 调试编辑器（C#）

如果要调试 C# 编辑器：

1. **直接启动 FirstEngineEditor**：
   - 设置 `FirstEngineEditor` 为启动项目
   - 按 F5 启动

2. **或者通过 FirstEngine 启动**：
   - 设置 `FirstEngine` 为启动项目
   - 命令行参数：`--editor`
   - 按 F5 启动
   - 编辑器启动后，可以在 Visual Studio 中附加到 `FirstEngineEditor.exe` 进程

## 附加到子进程

如果需要在编辑器启动后调试编辑器代码：

1. **调试 → 附加到进程** (Attach to Process)
2. **找到 `FirstEngineEditor.exe` 进程**
3. **点击"附加"**
4. **选择调试类型**：
   - ✅ **托管** (Managed) - 用于 C# 代码
   - ✅ **本机** (Native) - 用于 C++ DLL

## 验证清单

- [ ] `RenderApp.cpp` 已更新（等待子进程）
- [ ] Visual Studio 配置：**Debug x64**
- [ ] 启动项目：**FirstEngine**
- [ ] 命令行参数：`--editor`（如果使用编辑器模式）
- [ ] 断点设置在 `CreateProcessA` 调用前后
- [ ] 调试器不会退出

## 如果仍然有问题

### 方法 1：直接启动编辑器

不通过 `FirstEngine --editor`，直接启动编辑器：
- 设置 `FirstEngineEditor` 为启动项目
- 按 F5 启动

### 方法 2：使用条件断点

在 `CreateProcessA` 后设置条件断点，确保代码执行到该位置。

### 方法 3：添加调试输出

在代码中添加 `std::cout` 或 `OutputDebugString` 来验证代码执行路径。
