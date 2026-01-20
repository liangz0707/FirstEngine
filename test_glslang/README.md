# glslang 独立测试工程

这个工程用于测试 glslang 在 MSVC/VS2022 上的编译问题。

## 构建步骤

```powershell
# 创建构建目录
cd test_glslang
mkdir build
cd build

# 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release
```

## 测试

编译成功后，运行：

```powershell
.\bin\Release\test_glslang.exe
```

## 测试结果

- **glslang 13.1.1 + 静态库模式 (BUILD_SHARED_LIBS=OFF)**: ✅ 编译成功
- **glslang 13.1.1 + 动态库模式 (BUILD_SHARED_LIBS=ON)**: ⚠️ 存在 DLL 导入库链接问题

## 配置选项

可以在 `CMakeLists.txt` 中修改以下选项来测试不同的配置：

- `BUILD_SHARED_LIBS`: ON/OFF (测试静态库 vs 动态库)
  - **当前配置**: OFF (静态库模式，编译成功)
- `/Zp1` 编译器选项: 测试结构体对齐问题（已注释，需要时可启用）

## 结论

glslang 13.1.1 在 VS2022 上使用**静态库模式**可以正常编译，没有 `static_assert` 错误。
