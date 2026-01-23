# PowerShell script to generate simple test textures using .NET Graphics
# This creates basic colored PNG images for testing stb_image loading

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

# Function to create a simple colored PNG image
function New-TestImage {
    param(
        [string]$OutputPath,
        [int]$Width = 512,
        [int]$Height = 512,
        [string]$Color = "Blue",
        [string]$Text = ""
    )
    
    try {
        Add-Type -AssemblyName System.Drawing
        
        $bitmap = New-Object System.Drawing.Bitmap($Width, $Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        
        # Set high quality rendering
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        
        # Fill background with color
        $brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::$Color)
        $graphics.FillRectangle($brush, 0, 0, $Width, $Height)
        
        # Add text if specified
        if ($Text -ne "") {
            $font = New-Object System.Drawing.Font("Arial", 24, [System.Drawing.FontStyle]::Bold)
            $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
            $textSize = $graphics.MeasureString($Text, $font)
            $x = ($Width - $textSize.Width) / 2
            $y = ($Height - $textSize.Height) / 2
            $graphics.DrawString($Text, $font, $textBrush, $x, $y)
        }
        
        # Save as PNG
        $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
        
        $graphics.Dispose()
        $bitmap.Dispose()
        $brush.Dispose()
        if ($Text -ne "") {
            $textBrush.Dispose()
            $font.Dispose()
        }
        
        Write-Host "Generated: $OutputPath" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Failed to generate $OutputPath : $_" -ForegroundColor Red
        return $false
    }
}

Write-Host "`nGenerating test textures..." -ForegroundColor Cyan
Write-Host "==============================================`n" -ForegroundColor Cyan

$textures = @(
    @{
        Name = "example_texture.png"
        Color = "DodgerBlue"
        Text = "Example"
        Description = "Example test texture"
    },
    @{
        Name = "albedo_texture.png"
        Color = "Crimson"
        Text = "Albedo"
        Description = "Albedo texture (base color)"
    },
    @{
        Name = "normal_texture.png"
        Color = "MediumSlateBlue"
        Text = "Normal"
        Description = "Normal map texture"
    },
    @{
        Name = "normal_map.png"
        Color = "MediumSlateBlue"
        Text = "NormalMap"
        Description = "Normal map"
    },
    @{
        Name = "metallic_roughness_texture.png"
        Color = "MediumTurquoise"
        Text = "Metallic"
        Description = "Metallic roughness texture"
    },
    @{
        Name = "metallic_roughness.png"
        Color = "MediumTurquoise"
        Text = "MR"
        Description = "Metallic roughness"
    },
    @{
        Name = "ao_texture.png"
        Color = "Gray"
        Text = "AO"
        Description = "Ambient occlusion texture (grayscale)"
    },
    @{
        Name = "lightmap.png"
        Color = "MediumOrchid"
        Text = "Lightmap"
        Description = "Lightmap texture"
    },
    @{
        Name = "shadow_texture.png"
        Color = "DarkGray"
        Text = "Shadow"
        Description = "Shadow texture (grayscale)"
    },
    @{
        Name = "hdr_texture.png"
        Color = "Pink"
        Text = "HDR"
        Description = "HDR texture placeholder"
    }
)

$successCount = 0
$failCount = 0

foreach ($texture in $textures) {
    $filePath = Join-Path $texturesDir $texture.Name
    
    # Backup existing file if it exists
    if (Test-Path $filePath) {
        $backupPath = Join-Path $backupDir $texture.Name
        Copy-Item $filePath $backupPath -Force
        Write-Host "Backed up: $($texture.Name)" -ForegroundColor Gray
    }
    
    # Generate new image
    Write-Host "Generating: $($texture.Name)..." -ForegroundColor Yellow
    if (New-TestImage -OutputPath $filePath -Width 512 -Height 512 -Color $texture.Color -Text $texture.Text) {
        $successCount++
        Write-Host "  -> $($texture.Description)" -ForegroundColor Green
    }
    else {
        $failCount++
        # Restore backup if generation failed
        if (Test-Path (Join-Path $backupDir $texture.Name)) {
            Copy-Item (Join-Path $backupDir $texture.Name) $filePath -Force
            Write-Host "  -> Restored from backup" -ForegroundColor Yellow
        }
    }
    
    Write-Host ""
}

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "Generation Summary:" -ForegroundColor Cyan
Write-Host "  Success: $successCount" -ForegroundColor Green
Write-Host "  Failed:  $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Green" })
Write-Host "  Backup:  $backupDir" -ForegroundColor Gray
Write-Host "`nAll textures are 512x512 PNG format, compatible with stb_image." -ForegroundColor Green
Write-Host "Note: These are simple test images. For production use, replace with actual texture files." -ForegroundColor Yellow
