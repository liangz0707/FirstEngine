# 快速修复 C++ 调试器退出问题

## 问题

使用 `FirstEngine --editor` 启动时，执行 `CreateProcessA` 后 Visual Studio 调试器退出。

## 原因

代码在创建子进程后立即 `return 0`，导致主进程退出，调试器也随之退出。

## 已完成的修复

✅ 已修改 `RenderApp.cpp`：
- 创建子进程后，使用 `WaitForSingleObject` 等待子进程结束
- 获取子进程的退出码并返回
- 保持主进程存活，调试器不会退出

## 现在请执行

### 步骤 1：重新编译 C++ 项目

在 Visual Studio 中：
1. 右键解决方案 → **"重新生成解决方案"** (Rebuild Solution)

或在命令行：
```powershell
cd build
cmake --build . --config Debug
```

### 步骤 2：设置命令行参数（如果使用编辑器模式）

1. **右键 `FirstEngine` 项目 → 属性**
2. **配置属性 → 调试 → 命令参数**：
   - 输入：`--editor`

### 步骤 3：设置断点并测试

1. **在 `RenderApp.cpp` 中设置断点**：
   ```cpp
   if (CreateProcessA(...)) {
       CloseHandle(pi.hThread);
       
       // 👈 在这里设置断点
       WaitForSingleObject(pi.hProcess, INFINITE);
       // ...
   }
   ```

2. **确认 Visual Studio 配置**：
   - 配置：**Debug**
   - 平台：**x64**
   - 启动项目：**FirstEngine**

3. **按 F5 启动调试**

4. **调试器应该不会退出**，可以在断点处停止

## 验证

- [ ] 代码已重新编译
- [ ] 断点设置在 `WaitForSingleObject` 之前
- [ ] 按 F5 启动调试
- [ ] 调试器不会退出，可以在断点处停止

## 如果仍然退出

检查：
1. 是否使用了最新的代码（已包含 `WaitForSingleObject`）
2. 是否重新编译了项目
3. 断点是否设置在正确的位置
