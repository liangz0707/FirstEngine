# GitHub仓库创建和推送脚本
# 使用方法：.\create_and_push_github.ps1 -GitHubUsername "你的用户名" -RepositoryName "FirstEngine"

param(
    [Parameter(Mandatory=$false)]
    [string]$GitHubUsername = "",
    
    [Parameter(Mandatory=$false)]
    [string]$RepositoryName = "FirstEngine",
    
    [Parameter(Mandatory=$false)]
    [string]$GitHubToken = ""
)

Write-Host "=== GitHub仓库创建和推送工具 ===" -ForegroundColor Cyan
Write-Host ""

# 检查Git配置
$gitUser = git config --get user.name
$gitEmail = git config --get user.email

if (-not $gitUser -or -not $gitEmail) {
    Write-Host "错误: 请先配置Git用户信息" -ForegroundColor Red
    Write-Host "执行以下命令配置:" -ForegroundColor Yellow
    Write-Host "  git config --global user.name `"你的名字`"" -ForegroundColor Yellow
    Write-Host "  git config --global user.email `"你的邮箱`"" -ForegroundColor Yellow
    exit 1
}

Write-Host "Git配置信息:" -ForegroundColor Green
Write-Host "  用户名: $gitUser" -ForegroundColor White
Write-Host "  邮箱: $gitEmail" -ForegroundColor White
Write-Host ""

# 如果没有提供用户名，尝试从邮箱提取
if (-not $GitHubUsername) {
    if ($gitEmail -match '@') {
        $GitHubUsername = $gitEmail.Split('@')[0]
        Write-Host "从邮箱提取GitHub用户名: $GitHubUsername" -ForegroundColor Yellow
        Write-Host ""
    } else {
        Write-Host "错误: 无法确定GitHub用户名，请使用 -GitHubUsername 参数指定" -ForegroundColor Red
        exit 1
    }
}

# 如果没有提供Token，提示用户
if (-not $GitHubToken) {
    Write-Host "提示: 创建GitHub仓库需要Personal Access Token (PAT)" -ForegroundColor Yellow
    Write-Host "如果没有Token，请按照以下步骤操作:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "方法1: 使用GitHub CLI (推荐)" -ForegroundColor Cyan
    Write-Host "  1. 安装GitHub CLI: https://cli.github.com/" -ForegroundColor White
    Write-Host "  2. 运行: gh auth login" -ForegroundColor White
    Write-Host "  3. 运行: gh repo create $RepositoryName --public --source=. --remote=origin --push" -ForegroundColor White
    Write-Host ""
    Write-Host "方法2: 手动创建仓库" -ForegroundColor Cyan
    Write-Host "  1. 访问: https://github.com/new" -ForegroundColor White
    Write-Host "  2. 仓库名称: $RepositoryName" -ForegroundColor White
    Write-Host "  3. 选择 Public 或 Private" -ForegroundColor White
    Write-Host "  4. 不要初始化README、.gitignore或license" -ForegroundColor White
    Write-Host "  5. 点击 'Create repository'" -ForegroundColor White
    Write-Host "  6. 复制仓库地址，然后运行:" -ForegroundColor White
    Write-Host "     git remote add origin https://github.com/$GitHubUsername/$RepositoryName.git" -ForegroundColor Green
    Write-Host "     git push -u origin main" -ForegroundColor Green
    Write-Host ""
    
    $useManual = Read-Host "是否使用手动方法创建仓库? (Y/N)"
    if ($useManual -eq 'Y' -or $useManual -eq 'y') {
        Write-Host ""
        Write-Host "请按照上面的步骤在GitHub上创建仓库，然后告诉我仓库地址。" -ForegroundColor Yellow
        Write-Host "或者，如果你已经创建了仓库，请输入完整的仓库地址:" -ForegroundColor Yellow
        $repoUrl = Read-Host "仓库地址 (例如: https://github.com/username/repo.git)"
        
        if ($repoUrl) {
            Write-Host ""
            Write-Host "配置远程仓库..." -ForegroundColor Cyan
            git remote remove origin 2>$null
            git remote add origin $repoUrl
            
            Write-Host "推送到GitHub..." -ForegroundColor Cyan
            git push -u origin main
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "✅ 成功推送到GitHub!" -ForegroundColor Green
                Write-Host "仓库地址: $repoUrl" -ForegroundColor Cyan
            } else {
                Write-Host ""
                Write-Host "❌ 推送失败，请检查网络连接和权限" -ForegroundColor Red
            }
        }
        exit 0
    }
    
    # 尝试使用GitHub API（需要Token）
    Write-Host ""
    Write-Host "方法3: 使用GitHub API自动创建 (需要Personal Access Token)" -ForegroundColor Cyan
    Write-Host "获取Token: https://github.com/settings/tokens" -ForegroundColor White
    Write-Host "需要的权限: repo (完整仓库权限)" -ForegroundColor White
    Write-Host ""
    $GitHubToken = Read-Host "请输入GitHub Personal Access Token (留空跳过)"
}

# 如果提供了Token，尝试使用API创建仓库
if ($GitHubToken) {
    Write-Host ""
    Write-Host "使用GitHub API创建仓库..." -ForegroundColor Cyan
    
    $headers = @{
        "Authorization" = "token $GitHubToken"
        "Accept" = "application/vnd.github.v3+json"
    }
    
    $body = @{
        name = $RepositoryName
        description = "Modern 3D Graphics Engine in C++ using Vulkan and GLFW"
        private = $false
        auto_init = $false
    } | ConvertTo-Json
    
    try {
        $response = Invoke-RestMethod -Uri "https://api.github.com/user/repos" `
            -Method Post `
            -Headers $headers `
            -Body $body `
            -ContentType "application/json"
        
        $repoUrl = $response.clone_url
        Write-Host "✅ 仓库创建成功!" -ForegroundColor Green
        Write-Host "仓库地址: $repoUrl" -ForegroundColor Cyan
        Write-Host ""
        
        # 配置远程仓库
        Write-Host "配置远程仓库..." -ForegroundColor Cyan
        git remote remove origin 2>$null
        git remote add origin $repoUrl
        
        # 推送代码
        Write-Host "推送到GitHub..." -ForegroundColor Cyan
        git push -u origin main
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "✅ 成功推送到GitHub!" -ForegroundColor Green
            Write-Host "仓库地址: $repoUrl" -ForegroundColor Cyan
        } else {
            Write-Host ""
            Write-Host "❌ 推送失败，请检查网络连接" -ForegroundColor Red
        }
    } catch {
        Write-Host ""
        Write-Host "❌ 创建仓库失败: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "请检查Token是否有效，或使用手动方法创建仓库" -ForegroundColor Yellow
    }
} else {
    Write-Host ""
    Write-Host "请按照上面的方法之一创建GitHub仓库" -ForegroundColor Yellow
}
