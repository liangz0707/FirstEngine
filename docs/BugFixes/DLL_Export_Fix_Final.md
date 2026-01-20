# DLL 函数导出问题修复（历史方案）

> **注意**：本文档记录的是历史修复方案，当前推荐使用 [DLL导出问题修复](./DLL_Export_Fix.md) 中的方案。

## 历史方案说明

本文档记录了之前尝试的修复方案，但由于 CMake 配置问题，最终采用了更简单的 `EDITOR_API_EXPORT` 宏方案。

## 历史方案：移除实现文件中的 FE_CORE_API

### 原理

在 Windows DLL 中：
1. **头文件** (`EditorAPI.h`) 中声明：`FE_CORE_API void* EditorAPI_CreateEngine(...);`
2. **实现文件** (`EditorAPI.cpp`) 中定义：`void* EditorAPI_CreateEngine(...) { ... }`（不包含 `FE_CORE_API`）

### 问题

此方案要求 CMake 正确设置 `FirstEngine_Core_EXPORTS`，但在某些情况下可能失效，导致函数不被导出。

## 当前推荐方案

请参考 [DLL导出问题修复](./DLL_Export_Fix.md) 中的 `EDITOR_API_EXPORT` 方案，这是更可靠的解决方案。
