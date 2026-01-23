# Edit and Continue 修复

## 问题描述

Visual Studio 报错：
```
'Scene.cpp' in 'FirstEngine_Core.dll' was not compiled with Edit and Continue enabled. 
Ensure that the file is compiled with the Program Database for Edit and Continue (/ZI) 
option at Project Properties > C/C++ > Debug Information Format.
```

## 原因

Edit and Continue 需要 `/ZI` 编译器选项（Program Database for Edit and Continue），但项目使用的是 `/Zi`（普通 Program Database）。

- `/Zi`: 生成调试信息，但不支持 Edit and Continue
- `/ZI`: 生成支持 Edit and Continue 的调试信息

## 修复

已在根 `CMakeLists.txt` 中将 `/Zi` 改为 `/ZI`：

```cmake
# Ensure proper debug flags for MSVC
if(MSVC)
    # Set debug compilation flags
    # /ZI enables Edit and Continue (Program Database for Edit and Continue)
    # /Od disables optimizations for debugging
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /Od /D_DEBUG")
    # ...
endif()
```

## 应用修复的步骤

1. **重新生成 CMake 配置**：
   - 在 Visual Studio 中：右键解决方案 → "重新生成解决方案"
   - 或使用命令行：
     ```bash
     cd build
     cmake ..
     ```

2. **重新编译项目**：
   - 在 Visual Studio 中：生成 → 重新生成解决方案
   - 或使用命令行：
     ```bash
     cmake --build . --config Debug
     ```

3. **验证修复**：
   - 启动调试
   - 尝试在调试时修改代码
   - Edit and Continue 现在应该可以正常工作

## 注意事项

1. **性能影响**：`/ZI` 会生成更大的 PDB 文件，编译时间可能稍长
2. **优化限制**：Edit and Continue 需要禁用优化（`/Od`），这已经包含在配置中
3. **仅 Debug 配置**：此设置仅应用于 Debug 配置，Release 配置不受影响

## 相关文件

- `CMakeLists.txt` (根目录) - 已更新 `/Zi` → `/ZI`
- 所有子模块的 CMakeLists.txt 会继承此设置
