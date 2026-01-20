# 恢复 FirstEngineEditor 调试配置脚本
# 如果重新生成项目后调试路径丢失，运行此脚本

$projectRoot = Split-Path -Parent $PSScriptRoot
$vcxprojUserPath = Join-Path $projectRoot "build\Editor\FirstEngineEditor.vcxproj.user"
$editorPath = Join-Path $projectRoot "Editor"

# 确保目录存在
$buildEditorDir = Join-Path $projectRoot "build\Editor"
if (-not (Test-Path $buildEditorDir)) {
    New-Item -ItemType Directory -Path $buildEditorDir -Force | Out-Null
}

# 创建 .vcxproj.user 文件内容
$config = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>$editorPath\bin\Debug\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>$editorPath\bin\Debug\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Release|x64'">
    <LocalDebuggerCommand>$editorPath\bin\Release\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>$editorPath\bin\Release\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
"@

# 写入文件
$config | Out-File -FilePath $vcxprojUserPath -Encoding UTF8 -Force

Write-Host "✅ 调试配置已恢复: $vcxprojUserPath" -ForegroundColor Green
Write-Host ""
Write-Host "配置内容:"
Write-Host "  Debug:   $editorPath\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
Write-Host "  Release: $editorPath\bin\Release\net8.0-windows\FirstEngineEditor.exe"
Write-Host ""
Write-Host "现在可以重新打开 Visual Studio 并测试调试。" -ForegroundColor Yellow
