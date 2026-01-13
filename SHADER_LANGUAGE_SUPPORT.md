# Shader 语言支持说明

## 当前状态

### ✅ 已支持
- **SPIR-V → GLSL/HLSL/MSL**: 通过SPIRV-Cross实现
- **加载SPIR-V文件**: 通过ShaderLoader实现

### ❌ 未支持（需要集成）
- **GLSL → SPIR-V**: 需要glslang或shaderc
- **HLSL → SPIR-V**: 需要DXC（DirectX Shader Compiler）或shaderc
- **MSL → SPIR-V**: MSL通常直接用于Metal，转换到SPIR-V需求较少

## 可用的Shader语法

当前你可以：
1. 手动编写GLSL，然后使用外部工具（glslc）编译为SPIR-V
2. 手动编写HLSL，然后使用外部工具（dxc）编译为SPIR-V
3. 直接使用SPIR-V字节码文件

## 推荐集成方案

### 方案1: shaderc（推荐）
- **优点**: Google开发，API友好，同时支持GLSL和HLSL
- **仓库**: https://github.com/google/shaderc
- **支持**: GLSL → SPIR-V, HLSL → SPIR-V

### 方案2: glslang + DXC
- **glslang**: GLSL → SPIR-V（Khronos Group官方）
- **DXC**: HLSL → SPIR-V（微软官方，功能强大）
- **优点**: 官方支持，功能完整

我将集成shaderc，因为它更易用且API统一。
