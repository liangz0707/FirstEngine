# 推送到 GitHub 的步骤

## 步骤 1: 创建 GitHub 仓库（如果还没有）

1. 访问 https://github.com 并登录
2. 点击右上角的 "+" 按钮，选择 "New repository"
3. 填写仓库信息：
   - **Repository name**: `FirstEngine`
   - **Description**: `Modern 3D Graphics Engine built with C++, Vulkan, and GLFW`
   - 选择 **Public** 或 **Private**
   - **不要**勾选 "Initialize this repository with a README"
   - **不要**勾选 "Add .gitignore"
   - **不要**选择 License
4. 点击 "Create repository"

## 步骤 2: 添加远程仓库并推送

创建仓库后，GitHub 会显示一个页面，其中包含推送代码的命令。

### 方法 1: 使用 HTTPS（推荐，简单）

```bash
# 添加远程仓库（替换 YOUR_USERNAME 为你的GitHub用户名）
git remote add origin https://github.com/YOUR_USERNAME/FirstEngine.git

# 推送代码到GitHub
git push -u origin main
```

### 方法 2: 使用 SSH（需要配置SSH密钥）

```bash
# 添加远程仓库（替换 YOUR_USERNAME 为你的GitHub用户名）
git remote add origin git@github.com:YOUR_USERNAME/FirstEngine.git

# 推送代码到GitHub
git push -u origin main
```

## 如果遇到认证问题

如果使用 HTTPS 推送时要求输入用户名和密码：
- 用户名：你的GitHub用户名
- 密码：需要使用 **Personal Access Token**（不是GitHub密码）

### 创建 Personal Access Token

1. 访问 https://github.com/settings/tokens
2. 点击 "Generate new token" → "Generate new token (classic)"
3. 填写说明，选择过期时间
4. 勾选 `repo` 权限
5. 点击 "Generate token"
6. **复制生成的token**（只显示一次！）
7. 推送时密码输入框粘贴这个token

## 验证推送成功

推送成功后，访问 `https://github.com/YOUR_USERNAME/FirstEngine` 应该能看到你的代码了。
