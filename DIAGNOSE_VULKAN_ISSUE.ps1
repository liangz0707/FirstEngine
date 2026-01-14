# 全面诊断 Vulkan 问题

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Vulkan 问题诊断工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. 检查 DLL 位置
Write-Host "1. 检查 vulkan-1.dll 位置..." -ForegroundColor Yellow
$dllLocations = @(
    "external\vulkan\vulkan-1.dll",
    "external\vulkan\Bin\vulkan-1.dll",
    "build\bin\Debug\vulkan-1.dll",
    "build\bin\Release\vulkan-1.dll",
    "C:\Windows\System32\vulkan-1.dll"
)

$foundDlls = @()
foreach ($loc in $dllLocations) {
    if (Test-Path $loc) {
        Write-Host "  [找到] $loc" -ForegroundColor Green
        $foundDlls += $loc
    }
}

if ($foundDlls.Count -eq 0) {
    Write-Host "  [未找到] vulkan-1.dll" -ForegroundColor Red
} else {
    Write-Host "  找到 $($foundDlls.Count) 个 DLL" -ForegroundColor Green
}

Write-Host ""

# 2. 检查 GLFW 版本
Write-Host "2. 检查 GLFW 版本..." -ForegroundColor Yellow
Push-Location "third_party\glfw"
try {
    $glfwVersion = git describe --tags 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  GLFW 版本: $glfwVersion" -ForegroundColor Green
    } else {
        Write-Host "  GLFW 版本: 未知" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  无法获取 GLFW 版本" -ForegroundColor Yellow
}
Pop-Location

Write-Host ""

# 3. 检查 Vulkan SDK
Write-Host "3. 检查 Vulkan SDK..." -ForegroundColor Yellow
if ($env:VULKAN_SDK) {
    Write-Host "  VULKAN_SDK: $env:VULKAN_SDK" -ForegroundColor Green
    if (Test-Path "$env:VULKAN_SDK\Include\vulkan\vulkan.h") {
        Write-Host "  [找到] Vulkan 头文件" -ForegroundColor Green
    } else {
        Write-Host "  [未找到] Vulkan 头文件" -ForegroundColor Red
    }
} else {
    Write-Host "  VULKAN_SDK: 未设置" -ForegroundColor Yellow
}

Write-Host ""

# 4. 检查 external/vulkan
Write-Host "4. 检查 external/vulkan 目录..." -ForegroundColor Yellow
if (Test-Path "external\vulkan") {
    Write-Host "  目录存在" -ForegroundColor Green
    
    $hasInclude = Test-Path "external\vulkan\Include"
    $hasLib = Test-Path "external\vulkan\Lib"
    $hasDll = Test-Path "external\vulkan\vulkan-1.dll"
    
    Write-Host "  Include: $(if ($hasInclude) { '[有]' } else { '[无]' })" -ForegroundColor $(if ($hasInclude) { "Green" } else { "Red" })
    Write-Host "  Lib: $(if ($hasLib) { '[有]' } else { '[无]' })" -ForegroundColor $(if ($hasLib) { "Green" } else { "Red" })
    Write-Host "  vulkan-1.dll: $(if ($hasDll) { '[有]' } else { '[无]' })" -ForegroundColor $(if ($hasDll) { "Green" } else { "Red" })
} else {
    Write-Host "  目录不存在" -ForegroundColor Red
}

Write-Host ""

# 5. 检查构建输出
Write-Host "5. 检查构建输出..." -ForegroundColor Yellow
if (Test-Path "build\bin\Debug") {
    $debugDll = Test-Path "build\bin\Debug\vulkan-1.dll"
    Write-Host "  Debug\vulkan-1.dll: $(if ($debugDll) { '[有]' } else { '[无]' })" -ForegroundColor $(if ($debugDll) { "Green" } else { "Yellow" })
}

if (Test-Path "build\bin\Release") {
    $releaseDll = Test-Path "build\bin\Release\vulkan-1.dll"
    Write-Host "  Release\vulkan-1.dll: $(if ($releaseDll) { '[有]' } else { '[无]' })" -ForegroundColor $(if ($releaseDll) { "Green" } else { "Yellow" })
}

Write-Host ""

# 6. 建议
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "建议的修复步骤:" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "external\vulkan\vulkan-1.dll")) {
    Write-Host "1. 复制 vulkan-1.dll 到项目:" -ForegroundColor Yellow
    Write-Host "   Copy-Item `"C:\Windows\System32\vulkan-1.dll`" `"external\vulkan\vulkan-1.dll`" -Force" -ForegroundColor White
    Write-Host ""
}

Write-Host "2. 重新生成 CMake 项目:" -ForegroundColor Yellow
Write-Host "   Remove-Item -Recurse -Force build" -ForegroundColor White
Write-Host "   mkdir build" -ForegroundColor White
Write-Host "   cd build" -ForegroundColor White
Write-Host "   cmake .. -G `"Visual Studio 17 2022`" -A x64" -ForegroundColor White
Write-Host ""

Write-Host "3. 重新编译:" -ForegroundColor Yellow
Write-Host "   cmake --build . --config Release" -ForegroundColor White
Write-Host ""

Write-Host "4. 检查输出目录是否有 DLL:" -ForegroundColor Yellow
Write-Host "   Test-Path `"build\bin\Release\vulkan-1.dll`"" -ForegroundColor White
Write-Host ""

Write-Host "5. 如果仍然失败，运行程序查看详细错误信息" -ForegroundColor Yellow
Write-Host ""
