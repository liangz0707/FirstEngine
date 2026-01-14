# PowerShell script to convert all source files from LF to CRLF
# This ensures consistent CRLF line endings for Windows development

Write-Host "Converting line endings from LF to CRLF for all source files..." -ForegroundColor Green

# File extensions to process
$extensions = @("*.cpp", "*.h", "*.hpp", "*.c", "*.cmake", "CMakeLists.txt", "*.ps1", "*.md", "*.txt")

$fileCount = 0
$errorCount = 0

foreach ($ext in $extensions) {
    $files = Get-ChildItem -Path . -Include $ext -Recurse -File | Where-Object {
        $_.FullName -notmatch "\\third_party\\" -and
        $_.FullName -notmatch "\\build\\" -and
        $_.FullName -notmatch "\\external\\" -and
        $_.FullName -notmatch "\\.git\\"
    }
    
    foreach ($file in $files) {
        try {
            # Read file content as raw bytes to preserve encoding
            $content = [System.IO.File]::ReadAllText($file.FullName)
            
            # Check if file has LF (but not CRLF)
            if ($content -match "`n" -and $content -notmatch "`r`n") {
                # Replace LF with CRLF
                $newContent = $content -replace "`n", "`r`n"
                
                # Write back with UTF8 encoding (no BOM) and CRLF line endings
                $utf8NoBom = New-Object System.Text.UTF8Encoding $false
                [System.IO.File]::WriteAllText($file.FullName, $newContent, $utf8NoBom)
                $fileCount++
                Write-Host "  Converted: $($file.FullName)" -ForegroundColor Yellow
            }
            elseif ($content -match "`r`n") {
                # Already has CRLF, skip
                continue
            }
        }
        catch {
            Write-Host "  Error processing $($file.FullName): $_" -ForegroundColor Red
            $errorCount++
        }
    }
}

Write-Host "`nConversion complete!" -ForegroundColor Green
Write-Host "  Files converted: $fileCount" -ForegroundColor Cyan
if ($errorCount -gt 0) {
    Write-Host "  Errors: $errorCount" -ForegroundColor Red
}

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "  1. Run: git add --renormalize ." -ForegroundColor Yellow
Write-Host "  2. Commit the changes" -ForegroundColor Yellow
