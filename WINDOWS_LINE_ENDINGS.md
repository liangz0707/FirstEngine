# Windows 换行符配置说明

## 问题描述

在 Windows 系统上使用 Visual Studio 编译器时，如果 C++ 源文件使用 LF（Unix 风格）换行符，可能会导致编译错误。Visual Studio 编译器期望使用 CRLF（Windows 风格）换行符。

## 解决方案

### 1. `.gitattributes` 配置

项目中的 `.gitattributes` 文件已经配置为：
- C++ 源文件（`.cpp`, `.h`, `.hpp`, `.c`）使用 `eol=crlf`
- CMake 文件（`CMakeLists.txt`, `*.cmake`）使用 `eol=crlf`
- 脚本文件（`.bat`, `.ps1`）使用 `eol=crlf`
- Shell 脚本（`.sh`）使用 `eol=lf`
- Markdown 文件（`.md`）使用 `eol=lf`

### 2. Git 配置

确保 Git 的 `core.autocrlf` 设置为 `true`（Windows 默认值）：
```bash
git config --global core.autocrlf true
```

### 3. 转换现有文件

如果现有文件使用 LF 换行符，可以使用提供的 PowerShell 脚本转换：

```powershell
.\fix_line_endings.ps1
```

然后重新规范化 Git 索引：
```bash
git add --renormalize .
```

### 4. 编辑器设置

确保你的编辑器（如 Visual Studio Code、Visual Studio）配置为：
- 使用 CRLF 作为换行符（Windows 默认）
- 保存文件时保持现有换行符格式

#### Visual Studio Code
在设置中确保：
```json
{
  "files.eol": "\r\n",
  "files.insertFinalNewline": true
}
```

#### Visual Studio
- 工具 → 选项 → 文本编辑器 → 高级
- 确保"保留现有行尾"已启用

## 验证

检查文件换行符格式：
```powershell
# 检查文件是否使用 CRLF
Get-Content file.cpp -Raw | Select-String "`r`n"
```

## 注意事项

1. **不要手动修改 `.gitattributes`**：该文件已正确配置，修改可能导致问题
2. **提交前检查**：确保所有 C++ 源文件都使用 CRLF
3. **跨平台协作**：如果团队中有 Linux/Mac 开发者，他们需要在 checkout 时自动转换（Git 会自动处理）

## 故障排除

如果仍然遇到换行符问题：

1. 检查 `.gitattributes` 配置是否正确
2. 运行 `git add --renormalize .` 重新规范化所有文件
3. 检查编辑器设置
4. 确保 Git 配置正确：`git config --get core.autocrlf` 应该返回 `true`
