# GitHub Repository Setup Script
param(
    [string]$GitHubUsername = "",
    [string]$RepositoryName = "FirstEngine"
)

Write-Host "=== GitHub Repository Setup ===" -ForegroundColor Cyan
Write-Host ""

# Check Git config
$gitUser = git config --get user.name
$gitEmail = git config --get user.email

Write-Host "Current Git Config:" -ForegroundColor Green
Write-Host "  Username: $gitUser" -ForegroundColor White
Write-Host "  Email: $gitEmail" -ForegroundColor White
Write-Host ""

# Get GitHub username
if (-not $GitHubUsername) {
    if ($gitEmail -match '@') {
        $GitHubUsername = $gitEmail.Split('@')[0]
        Write-Host "Extracted GitHub username from email: $GitHubUsername" -ForegroundColor Yellow
    } else {
        $GitHubUsername = Read-Host "Please enter your GitHub username"
    }
}

Write-Host ""
Write-Host "Please follow these steps:" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Visit https://github.com/new to create a new repository" -ForegroundColor Yellow
Write-Host "2. Repository name: $RepositoryName" -ForegroundColor White
Write-Host "3. Choose Public or Private" -ForegroundColor White
Write-Host "4. DO NOT check any initialization options (README, .gitignore, license)" -ForegroundColor White
Write-Host "5. Click 'Create repository'" -ForegroundColor White
Write-Host ""
Write-Host "After creating the repository, please enter the full repository URL:" -ForegroundColor Cyan
Write-Host "Example: https://github.com/$GitHubUsername/$RepositoryName.git" -ForegroundColor Gray
Write-Host ""

$repoUrl = Read-Host "Repository URL"

if ($repoUrl) {
    Write-Host ""
    Write-Host "Configuring remote repository..." -ForegroundColor Cyan
    
    # Remove existing origin if any
    git remote remove origin 2>$null
    
    # Add new remote repository
    git remote add origin $repoUrl
    
    Write-Host "Pushing to GitHub..." -ForegroundColor Cyan
    git push -u origin main
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Successfully pushed to GitHub!" -ForegroundColor Green
        Write-Host "Repository URL: $repoUrl" -ForegroundColor Cyan
    } else {
        Write-Host ""
        Write-Host "Push failed. Please check:" -ForegroundColor Red
        Write-Host "  1. Is the repository URL correct?" -ForegroundColor Yellow
        Write-Host "  2. Do you have push permissions?" -ForegroundColor Yellow
        Write-Host "  3. Is your network connection working?" -ForegroundColor Yellow
    }
} else {
    Write-Host "No repository URL provided. Exiting." -ForegroundColor Yellow
}
