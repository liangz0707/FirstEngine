# GitHub 推送脚本
# 使用方法: .\push_to_github.ps1 -GitHubUsername YOUR_USERNAME -RepoName FirstEngine

param(
    [Parameter(Mandatory=$true)]
    [string]$GitHubUsername,
    
    [Parameter(Mandatory=$false)]
    [string]$RepoName = "FirstEngine",
    
    [Parameter(Mandatory=$false)]
    [switch]$UseSSH = $false
)

Write-Host "=== 推送到 GitHub ===" -ForegroundColor Green
Write-Host ""

# 检查是否已经设置远程仓库
$remoteExists = git remote get-url origin 2>$null
if ($remoteExists) {
    Write-Host "检测到已存在的远程仓库: $remoteExists" -ForegroundColor Yellow
    $overwrite = Read-Host "是否要替换它? (y/N)"
    if ($overwrite -eq "y" -or $overwrite -eq "Y") {
        git remote remove origin
    } else {
        Write-Host "取消操作" -ForegroundColor Red
        exit 1
    }
}

# 构建远程仓库URL
if ($UseSSH) {
    $remoteUrl = "git@github.com:$GitHubUsername/$RepoName.git"
    Write-Host "使用 SSH 连接" -ForegroundColor Cyan
} else {
    $remoteUrl = "https://github.com/$GitHubUsername/$RepoName.git"
    Write-Host "使用 HTTPS 连接" -ForegroundColor Cyan
}

Write-Host "远程仓库URL: $remoteUrl" -ForegroundColor Yellow
Write-Host ""

# 添加远程仓库
Write-Host "添加远程仓库..." -ForegroundColor Green
git remote add origin $remoteUrl

if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 无法添加远程仓库" -ForegroundColor Red
    exit 1
}

# 推送代码
Write-Host "推送代码到 GitHub..." -ForegroundColor Green
Write-Host "分支: main" -ForegroundColor Yellow
Write-Host ""

git push -u origin main

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✓ 推送成功!" -ForegroundColor Green
    Write-Host "仓库地址: https://github.com/$GitHubUsername/$RepoName" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "✗ 推送失败" -ForegroundColor Red
    Write-Host ""
    Write-Host "可能的原因:" -ForegroundColor Yellow
    Write-Host "1. GitHub仓库不存在 - 请先在GitHub上创建仓库"
    Write-Host "2. 认证失败 - 如果使用HTTPS，需要使用Personal Access Token"
    Write-Host "3. 网络问题 - 请检查网络连接"
    Write-Host ""
    Write-Host "详细帮助请查看 GITHUB_PUSH.md" -ForegroundColor Cyan
}
