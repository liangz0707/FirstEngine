# PowerShell script to convert all C++ source files from LF to CRLF
# This fixes Windows compilation issues with Visual Studio

Write-Host "Converting line endings from LF to CRLF for C++ source files..." -ForegroundColor Green

$extensions = @("*.cpp", "*.h", "*.hpp", "*.c", "*.cmake", "CMakeLists.txt")

$fileCount = 0
$errorCount = 0

foreach ($ext in $extensions) {
    $files = Get-ChildItem -Path . -Include $ext -Recurse -File | Where-Object {
        $_.FullName -notmatch "\\third_party\\" -and
        $_.FullName -notmatch "\\build\\" -and
        $_.FullName -notmatch "\\external\\"
    }
    
    foreach ($file in $files) {
        try {
            # Read file content
            $content = Get-Content -Path $file.FullName -Raw -Encoding UTF8
            
            # Convert LF to CRLF (but avoid double conversion)
            if ($content -match "`r`n") {
                # Already has CRLF, skip
                continue
            }
            
            # Replace LF with CRLF
            $newContent = $content -replace "`n(?!`r)", "`r`n"
            
            # Only write if content changed
            if ($newContent -ne $content) {
                # Write with UTF8 encoding and CRLF line endings
                [System.IO.File]::WriteAllText($file.FullName, $newContent, [System.Text.UTF8Encoding]::new($false))
                $fileCount++
                Write-Host "  Converted: $($file.FullName)" -ForegroundColor Yellow
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

Write-Host "`nNote: After running this script, you may need to:" -ForegroundColor Yellow
Write-Host "  1. Run: git add --renormalize ." -ForegroundColor Yellow
Write-Host "  2. Commit the changes" -ForegroundColor Yellow
