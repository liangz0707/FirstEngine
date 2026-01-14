# FirstEngine External Dependencies Setup Script
# This script helps copy Vulkan SDK and Python to the project's external/ directory

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FirstEngine External Dependencies Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Create external directory if it doesn't exist
$externalDir = "external"
if (-not (Test-Path $externalDir)) {
    New-Item -ItemType Directory -Path $externalDir | Out-Null
    Write-Host "Created $externalDir directory" -ForegroundColor Green
}

# Setup Vulkan SDK
Write-Host "`n[1] Setting up Vulkan SDK..." -ForegroundColor Yellow
$vulkanTarget = "$externalDir\vulkan"

# Check if already exists
if (Test-Path $vulkanTarget) {
    Write-Host "  Vulkan SDK already exists in $vulkanTarget" -ForegroundColor Yellow
    $overwrite = Read-Host "  Overwrite? (y/n)"
    if ($overwrite -ne "y") {
        Write-Host "  Skipping Vulkan SDK setup" -ForegroundColor Gray
    } else {
        Remove-Item $vulkanTarget -Recurse -Force
    }
}

if (-not (Test-Path $vulkanTarget)) {
    # Try to find Vulkan SDK
    $vulkanSource = $null
    
    # Check environment variable
    if ($env:VULKAN_SDK -and (Test-Path $env:VULKAN_SDK)) {
        $vulkanSource = $env:VULKAN_SDK
        Write-Host "  Found VULKAN_SDK: $vulkanSource" -ForegroundColor Green
    } else {
        # Try common installation paths
        $commonPaths = @(
            "C:\VulkanSDK\1.3.224.1",
            "C:\VulkanSDK\1.3.250.0",
            "C:\VulkanSDK\1.3.275.0",
            "D:\VulkanSDK\1.3.224.1",
            "D:\VulkanSDK\1.3.250.0"
        )
        
        foreach ($path in $commonPaths) {
            if (Test-Path $path) {
                $vulkanSource = $path
                Write-Host "  Found Vulkan SDK at: $path" -ForegroundColor Green
                break
            }
        }
    }
    
    if ($vulkanSource) {
        Write-Host "  Copying Vulkan SDK from $vulkanSource to $vulkanTarget..." -ForegroundColor Yellow
        Write-Host "  This may take a few minutes..." -ForegroundColor Gray
        
        # Copy essential directories
        $essentialDirs = @("Include", "Lib", "Bin", "Config")
        foreach ($dir in $essentialDirs) {
            $src = Join-Path $vulkanSource $dir
            $dst = Join-Path $vulkanTarget $dir
            if (Test-Path $src) {
                Write-Host "    Copying $dir..." -ForegroundColor Gray
                Copy-Item -Path $src -Destination $dst -Recurse -Force
            }
        }
        
        # Copy important files (try multiple locations)
        $importantFiles = @("vulkan-1.dll", "vulkan-1.lib")
        foreach ($file in $importantFiles) {
            # Try root directory first
            $src = Join-Path $vulkanSource $file
            if (Test-Path $src) {
                Copy-Item -Path $src -Destination $vulkanTarget -Force
                Write-Host "    Copied $file from root" -ForegroundColor Gray
            } else {
                # Try Bin directory
                $src = Join-Path (Join-Path $vulkanSource "Bin") $file
                if (Test-Path $src) {
                    Copy-Item -Path $src -Destination $vulkanTarget -Force
                    Write-Host "    Copied $file from Bin directory" -ForegroundColor Gray
                } else {
                    Write-Host "    Warning: $file not found in $vulkanSource" -ForegroundColor Yellow
                }
            }
        }
        
        # Also copy vulkan-1.dll to Bin subdirectory if it exists
        $vulkanBinTarget = Join-Path $vulkanTarget "Bin"
        if (Test-Path $vulkanBinTarget) {
            $dllSrc = Join-Path $vulkanTarget "vulkan-1.dll"
            if (Test-Path $dllSrc) {
                Copy-Item -Path $dllSrc -Destination $vulkanBinTarget -Force
                Write-Host "    Copied vulkan-1.dll to Bin subdirectory" -ForegroundColor Gray
            }
        }
        
        Write-Host "  [OK] Vulkan SDK copied successfully" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Vulkan SDK not found" -ForegroundColor Red
        Write-Host "    Please install Vulkan SDK or set VULKAN_SDK environment variable" -ForegroundColor Yellow
        Write-Host "    Download: https://www.vulkan.org/tools" -ForegroundColor Gray
    }
} else {
    Write-Host "  [OK] Vulkan SDK already set up" -ForegroundColor Green
}

# Setup Python
Write-Host "`n[2] Setting up Python 3.9..." -ForegroundColor Yellow
$pythonTarget = "$externalDir\python"

# Check if already exists
if (Test-Path $pythonTarget) {
    Write-Host "  Python already exists in $pythonTarget" -ForegroundColor Yellow
    $overwrite = Read-Host "  Overwrite? (y/n)"
    if ($overwrite -ne "y") {
        Write-Host "  Skipping Python setup" -ForegroundColor Gray
    } else {
        Remove-Item $pythonTarget -Recurse -Force
    }
}

if (-not (Test-Path $pythonTarget)) {
    # Try to find Python
    $pythonSource = $null
    
    # Check default configured path
    if (Test-Path "D:\Python3.9") {
        $pythonSource = "D:\Python3.9"
        Write-Host "  Found Python at configured path: $pythonSource" -ForegroundColor Green
    } elseif ($env:PYTHON_ROOT_DIR -and (Test-Path $env:PYTHON_ROOT_DIR)) {
        $pythonSource = $env:PYTHON_ROOT_DIR
        Write-Host "  Found PYTHON_ROOT_DIR: $pythonSource" -ForegroundColor Green
    } else {
        # Try to find Python in PATH
        if (Get-Command python -ErrorAction SilentlyContinue) {
            $pythonExe = (Get-Command python).Source
            $pythonDir = Split-Path $pythonExe -Parent
            $version = python --version 2>&1
            if ($version -match "Python 3\.") {
                $pythonSource = $pythonDir
                Write-Host "  Found Python in PATH: $pythonSource" -ForegroundColor Green
            }
        }
        
        # Try common installation paths
        if (-not $pythonSource) {
            $commonPaths = @(
                "C:\Python39",
                "C:\Python310",
                "C:\Python311",
                "D:\Python3.9",
                "D:\Python310",
                "D:\Python311"
            )
            
            foreach ($path in $commonPaths) {
                if (Test-Path $path) {
                    $pythonSource = $path
                    Write-Host "  Found Python at: $path" -ForegroundColor Green
                    break
                }
            }
        }
    }
    
    if ($pythonSource) {
        Write-Host "  Copying Python from $pythonSource to $pythonTarget..." -ForegroundColor Yellow
        Write-Host "  This may take a few minutes..." -ForegroundColor Gray
        
        # Copy essential directories
        $essentialDirs = @("include", "libs", "DLLs", "Lib")
        foreach ($dir in $essentialDirs) {
            $src = Join-Path $pythonSource $dir
            $dst = Join-Path $pythonTarget $dir
            if (Test-Path $src) {
                Write-Host "    Copying $dir..." -ForegroundColor Gray
                Copy-Item -Path $src -Destination $dst -Recurse -Force
            }
        }
        
        # Copy essential files
        $essentialFiles = @("python.exe", "pythonw.exe", "python39.dll", "python3.dll")
        foreach ($file in $essentialFiles) {
            $src = Join-Path $pythonSource $file
            if (Test-Path $src) {
                Copy-Item -Path $src -Destination $pythonTarget -Force
            }
        }
        
        Write-Host "  [OK] Python copied successfully" -ForegroundColor Green
    } else {
        Write-Host "  [FAIL] Python 3.9 not found" -ForegroundColor Red
        Write-Host "    Please install Python 3.9+ or set PYTHON_ROOT_DIR environment variable" -ForegroundColor Yellow
        Write-Host "    Download: https://www.python.org/downloads/" -ForegroundColor Gray
    }
} else {
    Write-Host "  [OK] Python already set up" -ForegroundColor Green
}

# Summary
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Setup Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "External dependencies are now in the 'external' directory." -ForegroundColor Green
Write-Host "CMake will automatically use these local dependencies." -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Run: .\check_environment.ps1 (to verify)" -ForegroundColor White
Write-Host "  2. Configure CMake: cd build && cmake .." -ForegroundColor White
Write-Host "  3. Build the project" -ForegroundColor White
Write-Host ""
