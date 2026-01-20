# 完全清理并重新生成项目，确保符号文件匹配

$ErrorActionPreference = "Stop"

Write-Host "🧹 清理项目..." -ForegroundColor Yellow

# 清理 dotnet 项目
cd Editor
dotnet clean

# 删除 bin 和 obj 文件夹
Write-Host "🗑️  删除 bin 和 obj 文件夹..." -ForegroundColor Yellow
Remove-Item -Recurse -Force bin -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force obj -ErrorAction SilentlyContinue

Write-Host "🔨 重新生成项目..." -ForegroundColor Yellow
dotnet build -c Debug --no-incremental

# 验证文件
Write-Host "`n✅ 验证生成的文件..." -ForegroundColor Green
$exe = Get-Item "bin\Debug\net8.0-windows\FirstEngineEditor.exe" -ErrorAction SilentlyContinue
$pdb = Get-Item "bin\Debug\net8.0-windows\FirstEngineEditor.pdb" -ErrorAction SilentlyContinue

if ($exe -and $pdb) {
    Write-Host "  EXE: $($exe.LastWriteTime) ($($exe.Length) bytes)" -ForegroundColor Green
    Write-Host "  PDB: $($pdb.LastWriteTime) ($($pdb.Length) bytes)" -ForegroundColor Green
    
    if ($exe.LastWriteTime -eq $pdb.LastWriteTime) {
        Write-Host "`n✅ 符号文件时间戳匹配！" -ForegroundColor Green
    } else {
        Write-Host "`n⚠️  符号文件时间戳不匹配" -ForegroundColor Yellow
    }
} else {
    Write-Host "❌ 文件未找到" -ForegroundColor Red
}

Write-Host "`n📝 下一步：" -ForegroundColor Cyan
Write-Host "  1. 在 Visual Studio 中：工具 → 选项 → 调试 → 符号" -ForegroundColor White
Write-Host "  2. 添加符号搜索路径：$PWD\bin\Debug\net8.0-windows" -ForegroundColor White
Write-Host "  3. 点击 '清空符号缓存'" -ForegroundColor White
Write-Host "  4. 重新启动调试（F5）" -ForegroundColor White
