# 推送到GitHub的命令

## 如果还没有创建GitHub仓库

1. 在GitHub上创建新仓库（不要初始化README）
2. 复制仓库地址（例如：https://github.com/username/FirstEngine.git）

## 配置远程仓库并推送

```bash
# 添加远程仓库（替换为你的仓库地址）
git remote add origin https://github.com/你的用户名/FirstEngine.git

# 或者使用SSH（如果你配置了SSH密钥）
git remote add origin git@github.com:你的用户名/FirstEngine.git

# 推送所有提交到GitHub
git push -u origin main
```

## 已提交的内容

✅ **4个提交已准备好推送**：

1. `feat(shader): 集成SPIRV-Cross库支持SPIR-V到GLSL/HLSL/MSL转换`
2. `feat(shader): 集成glslang库支持GLSL/HLSL到SPIR-V编译`
3. `docs(shader): 添加shader语法支持和功能文档`
4. `docs: 添加GitHub推送相关文档和脚本`

## 查看提交历史

```bash
git log --oneline -5
```

## 如果需要修改远程仓库地址

```bash
# 查看当前远程仓库
git remote -v

# 修改远程仓库地址
git remote set-url origin 新的仓库地址
```
