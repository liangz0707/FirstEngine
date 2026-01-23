# PowerShell script to download test textures for stb_image supported formats
# This script downloads sample images from public sources and replaces existing textures

$ErrorActionPreference = "Stop"

# Package Textures directory
$texturesDir = "build\Package\Textures"
if (-not (Test-Path $texturesDir)) {
    Write-Host "Error: Textures directory not found at $texturesDir" -ForegroundColor Red
    exit 1
}

# Create backup directory
$backupDir = "$texturesDir\backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
New-Item -ItemType Directory -Path $backupDir -Force | Out-Null
Write-Host "Created backup directory: $backupDir" -ForegroundColor Green

# Function to download image
function Download-Image {
    param(
        [string]$Url,
        [string]$OutputPath
    )
    
    try {
        Write-Host "Downloading: $Url" -ForegroundColor Yellow
        $webClient = New-Object System.Net.WebClient
        $webClient.DownloadFile($Url, $OutputPath)
        Write-Host "Downloaded: $OutputPath" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Failed to download $Url : $_" -ForegroundColor Red
        return $false
    }
    finally {
        if ($webClient) {
            $webClient.Dispose()
        }
    }
}

# Sample images from public sources
# Using placeholder services and free texture resources
# Note: These are test images - replace with actual textures for production

$downloads = @(
    @{
        Name = "example_texture.png"
        Url = "https://via.placeholder.com/512x512/4A90E2/FFFFFF.png?text=Example"
        Description = "Example test texture (512x512 PNG)"
    },
    @{
        Name = "albedo_texture.png"
        Url = "https://via.placeholder.com/512x512/FF6B6B/FFFFFF.png?text=Albedo"
        Description = "Albedo texture (512x512 PNG)"
    },
    @{
        Name = "normal_texture.png"
        Url = "https://via.placeholder.com/512x512/8080FF/FFFFFF.png?text=Normal"
        Description = "Normal map texture (512x512 PNG)"
    },
    @{
        Name = "normal_map.png"
        Url = "https://via.placeholder.com/512x512/8080FF/FFFFFF.png?text=NormalMap"
        Description = "Normal map (512x512 PNG)"
    },
    @{
        Name = "metallic_roughness_texture.png"
        Url = "https://via.placeholder.com/512x512/95E1D3/FFFFFF.png?text=Metallic"
        Description = "Metallic roughness texture (512x512 PNG)"
    },
    @{
        Name = "metallic_roughness.png"
        Url = "https://via.placeholder.com/512x512/95E1D3/FFFFFF.png?text=MR"
        Description = "Metallic roughness (512x512 PNG)"
    },
    @{
        Name = "ao_texture.png"
        Url = "https://via.placeholder.com/512x512/808080/FFFFFF.png?text=AO"
        Description = "Ambient occlusion texture (512x512 grayscale PNG)"
    },
    @{
        Name = "lightmap.png"
        Url = "https://via.placeholder.com/512x512/AA96DA/FFFFFF.png?text=Lightmap"
        Description = "Lightmap texture (512x512 PNG)"
    },
    @{
        Name = "shadow_texture.png"
        Url = "https://via.placeholder.com/512x512/2C2C2C/FFFFFF.png?text=Shadow"
        Description = "Shadow texture (512x512 grayscale PNG)"
    },
    @{
        Name = "hdr_texture.png"
        Url = "https://via.placeholder.com/512x512/FCBAD3/FFFFFF.png?text=HDR"
        Description = "HDR texture placeholder (512x512 PNG)"
        Note = "Note: Real HDR textures should use .hdr format, not .png"
    }
)

Write-Host "`nStarting texture download and replacement..." -ForegroundColor Cyan
Write-Host "==============================================`n" -ForegroundColor Cyan

$successCount = 0
$failCount = 0

foreach ($download in $downloads) {
    $filePath = Join-Path $texturesDir $download.Name
    
    # Backup existing file if it exists
    if (Test-Path $filePath) {
        $backupPath = Join-Path $backupDir $download.Name
        Copy-Item $filePath $backupPath -Force
        Write-Host "Backed up: $($download.Name)" -ForegroundColor Gray
    }
    
    # Download new image
    if (Download-Image -Url $download.Url -OutputPath $filePath) {
        $successCount++
        Write-Host "  -> $($download.Description)" -ForegroundColor Green
    }
    else {
        $failCount++
        # Restore backup if download failed
        if (Test-Path (Join-Path $backupDir $download.Name)) {
            Copy-Item (Join-Path $backupDir $download.Name) $filePath -Force
            Write-Host "  -> Restored from backup" -ForegroundColor Yellow
        }
    }
    
    Write-Host ""
}

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "Download Summary:" -ForegroundColor Cyan
Write-Host "  Success: $successCount" -ForegroundColor Green
Write-Host "  Failed:  $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Green" })
Write-Host "  Backup:  $backupDir" -ForegroundColor Gray
Write-Host "`nNote: These are placeholder images. For production use, replace with actual texture files." -ForegroundColor Yellow
