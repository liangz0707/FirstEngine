# C++ 调试设置指南

## 问题

C++ 代码无法断点调试。

## 已完成的修复

1. ✅ 已在 `CMakeLists.txt` 中添加了 MSVC 调试标志：
   - `/Zi` - 生成调试信息
   - `/Od` - 禁用优化
   - `/DEBUG` - 链接器生成调试信息

## 立即执行的步骤

### 步骤 1：重新配置 CMake（必须！）

由于修改了 `CMakeLists.txt`，需要重新配置：

```powershell
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

或者如果使用 Visual Studio 生成器：

```powershell
cmake .. -G "Visual Studio 17 2022" -A x64
```

### 步骤 2：清理并重新生成 C++ 项目

**在 Visual Studio 中**：
1. 右键解决方案 → **"清理解决方案"** (Clean Solution)
2. 等待完成
3. 右键解决方案 → **"重新生成解决方案"** (Rebuild Solution)

**或在命令行**：
```powershell
cd build
cmake --build . --config Debug --clean-first
```

### 步骤 3：验证 .pdb 文件生成

检查 .pdb 文件是否存在：

```powershell
cd build
Get-ChildItem -Recurse -Filter "*.pdb" | Where-Object { $_.Name -like "*FirstEngine*" } | Select-Object FullName, Length, LastWriteTime
```

应该看到多个 .pdb 文件，包括：
- `FirstEngine_Cored.pdb`
- `FirstEngine_Deviced.pdb`
- `FirstEngine_RHId.pdb`
- 等等...

### 步骤 4：在 Visual Studio 中设置断点

1. **打开 C++ 源文件**（如 `src/Application/RenderApp.cpp`）

2. **找到 `main` 函数**（在文件末尾，大约第 500 行）

3. **设置断点**：
   ```cpp
   int main(int argc, char* argv[])
   {
       int debugTest = 123;  // 👈 在这里设置断点
       // ...
   }
   ```

4. **检查断点状态**：
   - ✅ **实心红色圆圈** = 断点已绑定
   - ⚠️ **空心圆圈** = 断点未绑定

### 步骤 5：启动调试

1. **确认 Visual Studio 配置**：
   - 配置：**Debug**
   - 平台：**x64**
   - 启动项目：**FirstEngine**（不是 FirstEngineEditor）

2. **按 F5 启动调试**

3. **打开模块窗口**：
   - 调试 → 窗口 → 模块（`Ctrl+Alt+U`）

4. **检查符号状态**：
   - 找到 `FirstEngine.exe`
   - 找到 `FirstEngine_Cored.dll`
   - 检查 **"符号状态"** 列
   - 应该显示 **"已加载符号"**

### 步骤 6：如果符号未加载

1. **在模块窗口中，右键 DLL/EXE**
2. **选择 "加载符号"** (Load Symbols)
3. **浏览到对应的 .pdb 文件**：
   - `FirstEngine.exe` → `build\bin\Debug\FirstEngine.pdb`（如果存在）
   - `FirstEngine_Cored.dll` → `build\bin\Debug\FirstEngine_Cored.pdb`
   - `FirstEngine_Deviced.dll` → `build\lib\Debug\FirstEngine_Deviced.pdb`

### 步骤 7：配置符号搜索路径

1. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 符号 (Symbols)**

2. **添加符号搜索路径**：
   - `G:\AIHUMAN\WorkSpace\FirstEngine\build\bin\Debug`
   - `G:\AIHUMAN\WorkSpace\FirstEngine\build\lib\Debug`

3. **清除符号缓存**

4. **重新启动调试**

## 验证编译选项

在 Visual Studio 中验证编译选项：

1. **右键 C++ 项目**（如 `FirstEngine_Core`）→ 属性

2. **配置属性 → C/C++ → 常规**：
   - **调试信息格式**：应该是 **"程序数据库 (/Zi)"**
   - **优化**：应该是 **"禁用 (/Od)"**

3. **配置属性 → 链接器 → 调试**：
   - **生成调试信息**：应该是 **"是 (/DEBUG)"**

## 常见问题

### Q: 断点显示为空心圆圈

**A**: 可能原因：
- 代码已优化（Release 配置）
- 符号文件未生成或不匹配
- 源代码与编译的代码不一致

**解决**：
1. 确认是 Debug 配置
2. 重新配置 CMake 并重新生成
3. 检查 .pdb 文件是否存在

### Q: 可以调试主程序但无法调试 DLL

**A**: 需要确保：
1. DLL 也是 Debug 配置编译
2. DLL 的 .pdb 文件存在
3. 符号搜索路径包含 DLL 的 .pdb 文件位置

### Q: 提示"符号未加载"

**A**: 
1. 手动加载符号（见步骤 6）
2. 检查 .pdb 文件路径
3. 确认 .pdb 文件与 .dll/.exe 时间戳匹配

## 验证清单

- [ ] CMake 已重新配置（使用 Debug 配置）
- [ ] C++ 项目已清理并重新生成
- [ ] Visual Studio 配置：**Debug x64**
- [ ] 启动项目是 **FirstEngine**（C++ 可执行文件）
- [ ] 编译选项：**/Zi /Od**（调试信息，禁用优化）
- [ ] 链接器选项：**/DEBUG**（生成调试信息）
- [ ] .pdb 文件存在且时间戳匹配
- [ ] 符号搜索路径已配置
- [ ] 断点是实心红色圆圈
- [ ] 模块窗口中显示"已加载符号"

## 快速测试

1. **在 `RenderApp.cpp` 的 `main` 函数开头设置断点**：
   ```cpp
   int main(int argc, char* argv[])
   {
       int debugTest = 123;  // 👈 设置断点
       // ...
   }
   ```

2. **按 F5 启动调试**

3. **如果断点生效**：
   - 程序应该在断点处停止
   - 可以查看变量值
   - 可以单步执行

4. **如果仍然不生效**：
   - 查看模块窗口中的符号状态
   - 检查输出窗口的调试信息
   - 确认是 Debug 配置，不是 Release
