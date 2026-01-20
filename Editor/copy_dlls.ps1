# PowerShell script to copy DLLs and create non-suffixed copies for Debug configuration
param(
    [string]$Config = "Debug",
    [string]$SourceDir = "",
    [string]$DestDir = ""
)

if ([string]::IsNullOrEmpty($SourceDir) -or [string]::IsNullOrEmpty($DestDir)) {
    Write-Host "Usage: copy_dlls.ps1 -Config Debug -SourceDir <source> -DestDir <dest>"
    exit 1
}

# Ensure destination directory exists
New-Item -ItemType Directory -Force -Path $DestDir | Out-Null

# List of DLLs to copy
$dlls = @(
    "FirstEngine_Core",
    "FirstEngine_RHI",
    "FirstEngine_Shader"
)

foreach ($dll in $dlls) {
    $sourceFile = ""
    $destFile = ""
    
    if ($Config -eq "Debug") {
        # Debug: DLL has 'd' suffix
        $sourceFile = Join-Path $SourceDir "${dll}d.dll"
        $destFile = Join-Path $DestDir "${dll}d.dll"
    } else {
        # Release: No suffix
        $sourceFile = Join-Path $SourceDir "${dll}.dll"
        $destFile = Join-Path $DestDir "${dll}.dll"
    }
    
    if (Test-Path $sourceFile) {
        # Copy the DLL
        Copy-Item -Path $sourceFile -Destination $destFile -Force
        Write-Host "Copied: $sourceFile -> $destFile"
        
        # In Debug, also create a copy without 'd' suffix for C# P/Invoke
        if ($Config -eq "Debug") {
            $destFileNoSuffix = Join-Path $DestDir "${dll}.dll"
            Copy-Item -Path $sourceFile -Destination $destFileNoSuffix -Force
            Write-Host "Copied (no suffix): $sourceFile -> $destFileNoSuffix"
        }
    } else {
        Write-Warning "DLL not found: $sourceFile"
    }
}

# Copy vulkan-1.dll if it exists
$vulkanDll = Join-Path $SourceDir "vulkan-1.dll"
if (Test-Path $vulkanDll) {
    $vulkanDest = Join-Path $DestDir "vulkan-1.dll"
    Copy-Item -Path $vulkanDll -Destination $vulkanDest -Force
    Write-Host "Copied: $vulkanDll -> $vulkanDest"
}
