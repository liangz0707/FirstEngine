# GitHub 仓库设置指南

## 步骤 1: 在 GitHub 上创建新仓库

1. 访问 [GitHub](https://github.com)
2. 点击右上角的 "+" 按钮，选择 "New repository"
3. 填写仓库信息：
   - **Repository name**: `FirstEngine` (或你喜欢的名字)
   - **Description**: `Modern 3D Graphics Engine built with C++, Vulkan, and GLFW`
   - **Visibility**: 选择 Public 或 Private
   - **不要**勾选 "Initialize this repository with a README"（因为我们已经有文件了）
4. 点击 "Create repository"

## 步骤 2: 连接本地仓库到 GitHub

在项目根目录运行以下命令（替换 `YOUR_USERNAME` 和 `YOUR_REPO_NAME`）：

```bash
# 添加远程仓库（替换为你的仓库URL）
git remote add origin https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git

# 或者使用SSH（如果你配置了SSH密钥）
git remote add origin git@github.com:YOUR_USERNAME/YOUR_REPO_NAME.git

# 查看远程仓库配置
git remote -v

# 推送代码到GitHub
git branch -M main
git push -u origin main
```

## 步骤 3: 设置 Git 用户信息（如果还没设置）

如果你需要使用你的真实GitHub账户信息：

```bash
git config user.name "你的GitHub用户名"
git config user.email "你的GitHub邮箱"
```

或者全局设置：

```bash
git config --global user.name "你的GitHub用户名"
git config --global user.email "你的GitHub邮箱"
```

## 日常使用

### 提交更改
```bash
# 查看更改状态
git status

# 添加文件到暂存区
git add .

# 提交更改
git commit -m "描述你的更改"

# 推送到GitHub
git push
```

### 查看提交历史
```bash
git log --oneline
```

### 创建分支
```bash
# 创建新分支
git checkout -b feature/your-feature-name

# 切换分支
git checkout main

# 合并分支
git merge feature/your-feature-name
```

## 注意事项

- `.gitignore` 文件已经配置好，会忽略构建文件和编译产物
- 不要提交构建目录（`build/`）中的文件
- 定期提交和推送你的更改
- 使用有意义的提交信息

## 推荐的仓库结构

你的GitHub仓库应该包含：
- ✅ 所有源代码文件（`src/` 目录）
- ✅ 所有头文件（`include/` 目录）
- ✅ CMakeLists.txt 文件
- ✅ README.md 和 ARCHITECTURE.md
- ✅ .gitignore 和 .gitattributes
- ❌ 构建产物（`build/` 目录）会被自动忽略
- ❌ 编译后的 DLL 和 EXE 文件会被自动忽略
