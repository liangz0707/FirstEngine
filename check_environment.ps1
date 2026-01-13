# FirstEngine Environment Dependency Check Script
# Run this script to check all required system dependencies

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FirstEngine Environment Dependency Check" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$allOk = $true

# Check Python
Write-Host "`n[1] Checking Python 3.x..." -ForegroundColor Yellow
$pythonFound = $false
$pythonPath = ""

# Check Python in system PATH
if (Get-Command python -ErrorAction SilentlyContinue) {
    $version = python --version 2>&1
    if ($version -match "Python 3\.") {
        Write-Host "  [OK] Found: $version" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = (Get-Command python).Source
    } else {
        Write-Host "  [FAIL] Found Python but not 3.x: $version" -ForegroundColor Red
    }
}

# Check default configured path
if (-not $pythonFound) {
    $defaultPath = "D:\Python3.9"
    if (Test-Path $defaultPath) {
        Write-Host "  [OK] Found configured Python path: $defaultPath" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = $defaultPath
    }
}

# Check environment variable
if (-not $pythonFound -and $env:PYTHON_ROOT_DIR) {
    if (Test-Path $env:PYTHON_ROOT_DIR) {
        Write-Host "  [OK] Found PYTHON_ROOT_DIR: $env:PYTHON_ROOT_DIR" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = $env:PYTHON_ROOT_DIR
    }
}

if (-not $pythonFound) {
    Write-Host "  [FAIL] Python 3.x not found" -ForegroundColor Red
    Write-Host "    Please install Python 3.9+ or set PYTHON_ROOT_DIR environment variable" -ForegroundColor Yellow
    $allOk = $false
} else {
    Write-Host "    Path: $pythonPath" -ForegroundColor Gray
}

# Check Vulkan SDK
Write-Host "`n[2] Checking Vulkan SDK..." -ForegroundColor Yellow
if ($env:VULKAN_SDK) {
    if (Test-Path $env:VULKAN_SDK) {
        Write-Host "  [OK] Found: $env:VULKAN_SDK" -ForegroundColor Green
        if (Test-Path "$env:VULKAN_SDK\Include\vulkan\vulkan.h") {
            Write-Host "    [OK] Vulkan headers exist" -ForegroundColor Green
        } else {
            Write-Host "    [FAIL] Vulkan headers not found" -ForegroundColor Red
            $allOk = $false
        }
    } else {
        Write-Host "  [FAIL] VULKAN_SDK points to non-existent path" -ForegroundColor Red
        $allOk = $false
    }
} else {
    Write-Host "  [FAIL] VULKAN_SDK environment variable not set" -ForegroundColor Red
    Write-Host "    Please install Vulkan SDK and set environment variable" -ForegroundColor Yellow
    Write-Host "    Download: https://www.vulkan.org/tools" -ForegroundColor Gray
    $allOk = $false
}

# Check CMake
Write-Host "`n[3] Checking CMake..." -ForegroundColor Yellow
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $version = cmake --version | Select-Object -First 1
    Write-Host "  [OK] Found: $version" -ForegroundColor Green
    
    # Check version
    $versionMatch = $version -match "version (\d+)\.(\d+)"
    if ($versionMatch) {
        $major = [int]$matches[1]
        $minor = [int]$matches[2]
        if ($major -gt 3 -or ($major -eq 3 -and $minor -ge 20)) {
            Write-Host "    [OK] Version meets requirement (>= 3.20)" -ForegroundColor Green
        } else {
            Write-Host "    [FAIL] Version too low, need >= 3.20" -ForegroundColor Red
            $allOk = $false
        }
    }
} else {
    Write-Host "  [FAIL] CMake not found" -ForegroundColor Red
    Write-Host "    Please install CMake 3.20 or higher" -ForegroundColor Yellow
    $allOk = $false
}

# Check Visual Studio compiler
Write-Host "`n[4] Checking Visual Studio compiler..." -ForegroundColor Yellow
if (Get-Command cl -ErrorAction SilentlyContinue) {
    Write-Host "  [OK] Found Visual Studio compiler" -ForegroundColor Green
    $clVersion = cl 2>&1 | Select-Object -First 1
    Write-Host "    $clVersion" -ForegroundColor Gray
} else {
    Write-Host "  [FAIL] Visual Studio compiler not found" -ForegroundColor Red
    Write-Host "    Please use Visual Studio Developer Command Prompt" -ForegroundColor Yellow
    Write-Host "    Or open project in Visual Studio" -ForegroundColor Yellow
    $allOk = $false
}

# Check Git submodules
Write-Host "`n[5] Checking Git submodules..." -ForegroundColor Yellow
$submodules = @("third_party/glfw", "third_party/glm", "third_party/spirv-cross", 
                "third_party/glslang", "third_party/pybind11", "third_party/stb", 
                "third_party/assimp")
$submodulesOk = $true
foreach ($submodule in $submodules) {
    if (Test-Path $submodule) {
        Write-Host "  [OK] $submodule" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] $submodule not found" -ForegroundColor Red
        $submodulesOk = $false
        $allOk = $false
    }
}
if (-not $submodulesOk) {
    Write-Host "    Run: git submodule update --init --recursive" -ForegroundColor Yellow
}

# Summary
Write-Host "`n========================================" -ForegroundColor Cyan
if ($allOk) {
    Write-Host "[SUCCESS] All dependency checks passed!" -ForegroundColor Green
    Write-Host "  You can start building the project" -ForegroundColor Green
} else {
    Write-Host "[FAIL] Some dependencies are missing or misconfigured" -ForegroundColor Red
    Write-Host "  Please fix issues according to the prompts above" -ForegroundColor Yellow
    Write-Host "`nFor detailed information, see ENVIRONMENT.md" -ForegroundColor Cyan
}
Write-Host "========================================" -ForegroundColor Cyan
