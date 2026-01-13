# GitHub仓库创建和推送指南

## 快速步骤

### 1. 创建GitHub仓库

访问：**https://github.com/new**

填写信息：
- **Repository name**: `FirstEngine` (或你喜欢的名字)
- **Description**: `Modern 3D Graphics Engine in C++ using Vulkan and GLFW`
- **Visibility**: 选择 Public 或 Private
- **重要**: 不要勾选任何初始化选项（README、.gitignore、license）

点击 **"Create repository"**

### 2. 配置并推送

创建仓库后，GitHub会显示仓库地址，格式类似：
```
https://github.com/你的用户名/FirstEngine.git
```

然后执行以下命令（替换为你的实际仓库地址）：

```powershell
# 配置远程仓库
git remote add origin https://github.com/你的用户名/FirstEngine.git

# 推送到GitHub
git push -u origin main
```

### 3. 或者使用脚本

运行提供的脚本：
```powershell
.\setup_github.ps1
```

脚本会引导你输入仓库地址并自动配置推送。

## 已准备好的提交

你的代码已经分成4个清晰的提交，准备好推送：

1. ✅ `feat(shader): 集成SPIRV-Cross库支持SPIR-V到GLSL/HLSL/MSL转换`
2. ✅ `feat(shader): 集成glslang库支持GLSL/HLSL到SPIR-V编译`
3. ✅ `docs(shader): 添加shader语法支持和功能文档`
4. ✅ `docs: 添加GitHub推送相关文档和脚本`

## 如果遇到问题

### 推送被拒绝
如果提示需要认证，你可能需要：
1. 使用Personal Access Token (PAT) 代替密码
2. 或者配置SSH密钥

### 查看当前状态
```powershell
git status
git log --oneline -5
git remote -v
```
