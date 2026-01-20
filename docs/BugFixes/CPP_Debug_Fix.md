# 修复 C++ 调试断点问题

## 问题

C++ 代码无法断点调试，即使程序可以正常运行。

## 检查清单

### 1. 确认 Visual Studio 配置

- ✅ 配置：**Debug**（不是 Release）
- ✅ 平台：**x64**
- ✅ 启动项目：**FirstEngine**（C++ 可执行文件）

### 2. 检查 .pdb 文件

C++ 项目的 .pdb 文件应该在：
- `build\bin\Debug\FirstEngine_Cored.pdb`
- `build\bin\Debug\FirstEngine_RHId.pdb`
- `build\bin\Debug\FirstEngine_Shaderd.pdb`
- `build\lib\Debug\FirstEngine_Deviced.pdb`
- 等等...

### 3. 验证编译选项

在 Visual Studio 中：
1. 右键 C++ 项目（如 `FirstEngine_Core`）→ 属性
2. 配置属性 → C/C++ → 常规
3. 检查：
   - **调试信息格式**：应该是 **"程序数据库 (/Zi)"** 或 **"用于"编辑并继续"的程序数据库 (/ZI)"**
   - **优化**：应该是 **"禁用 (/Od)"**

4. 配置属性 → 链接器 → 调试
5. 检查：
   - **生成调试信息**：应该是 **"是 (/DEBUG)"**

## 解决方案

### 步骤 1：在 CMake 中确保调试配置

检查 `CMakeLists.txt` 是否正确设置了调试标志。对于 MSVC，需要：

```cmake
if(MSVC)
    # 确保 Debug 配置有正确的编译选项
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()
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

检查 .pdb 文件是否存在且时间戳与 .dll/.exe 匹配：

```powershell
cd build
Get-ChildItem -Recurse -Filter "*.pdb" | Where-Object { $_.Name -like "*FirstEngine*" } | Select-Object FullName, LastWriteTime
```

### 步骤 4：在 Visual Studio 中设置断点

1. **打开 C++ 源文件**（如 `src/Application/RenderApp.cpp`）

2. **在代码中设置断点**：
   ```cpp
   int main(int argc, char* argv[])
   {
       int test = 123;  // 👈 在这里设置断点
       // ...
   }
   ```

3. **检查断点状态**：
   - ✅ **实心红色圆圈** = 断点已绑定
   - ⚠️ **空心圆圈** = 断点未绑定，需要检查

### 步骤 5：启动调试并检查模块窗口

1. **按 F5 启动调试**

2. **打开模块窗口**：
   - 调试 → 窗口 → 模块（`Ctrl+Alt+U`）

3. **查找 C++ DLL/EXE**：
   - `FirstEngine.exe`
   - `FirstEngine_Cored.dll`
   - `FirstEngine_Deviced.dll`
   - 等等...

4. **检查符号状态**：
   - ✅ **"已加载符号"** = 正常
   - ⚠️ **"无法查找或打开 PDB 文件"** = 需要手动加载

5. **如果符号未加载**：
   - 右键 DLL/EXE
   - 选择 **"加载符号"** (Load Symbols)
   - 浏览到对应的 .pdb 文件位置

### 步骤 6：配置符号搜索路径

1. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 符号 (Symbols)**

2. **添加符号搜索路径**：
   - `G:\AIHUMAN\WorkSpace\FirstEngine\build\bin\Debug`
   - `G:\AIHUMAN\WorkSpace\FirstEngine\build\lib\Debug`

3. **清除符号缓存**

4. **重新启动调试**

## 常见问题

### Q: 断点显示为空心圆圈

**A**: 可能原因：
- 代码已优化（Release 配置）
- 符号文件未生成或不匹配
- 源代码与编译的代码不一致

**解决**：
1. 确认是 Debug 配置
2. 清理并重新生成
3. 检查 .pdb 文件是否存在

### Q: 可以调试主程序但无法调试 DLL

**A**: 需要确保：
1. DLL 也是 Debug 配置编译
2. DLL 的 .pdb 文件存在
3. 符号搜索路径包含 DLL 的 .pdb 文件位置

### Q: 提示"符号未加载"

**A**: 
1. 手动加载符号（见步骤 5）
2. 检查 .pdb 文件路径
3. 确认 .pdb 文件与 .dll/.exe 时间戳匹配

## 验证清单

- [ ] Visual Studio 配置：**Debug x64**
- [ ] 启动项目是 **FirstEngine**（C++ 可执行文件）
- [ ] C++ 项目编译选项：**/Zi /Od**（调试信息，禁用优化）
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
