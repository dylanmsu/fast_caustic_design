# PowerShell script for building a single, self-contained executable
# This script builds the caustic_design application as a standalone executable

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building Caustic Design Single Executable" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

# Setup Visual Studio environment
$vsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $vsPath)) {
    Write-Host "Visual Studio Build Tools not found at expected location!" -ForegroundColor Red
    Write-Host "Please ensure Visual Studio 2019 Build Tools are installed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Configure with single executable option
Write-Host "Configuring CMake for single executable build..." -ForegroundColor Green
$configCmd = "cmd /c `"call `"$vsPath`" && cmake .. -DBUILD_SINGLE_EXECUTABLE=ON -DCMAKE_BUILD_TYPE=Release`""
$configResult = Invoke-Expression $configCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Building single executable..." -ForegroundColor Green
Write-Host "This may take a few minutes..." -ForegroundColor Yellow

# Build the project
$buildCmd = "cmd /c `"call `"$vsPath`" && cmake --build . --config Release --target caustic_design`""
$buildResult = Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

# Show the result
$exePath = "Release\caustic_design_standalone.exe"
if (Test-Path $exePath) {
    Write-Host "Single executable created successfully:" -ForegroundColor Green
    Write-Host "  Location: build\$exePath" -ForegroundColor White
    
    # Get file size
    $fileSize = (Get-Item $exePath).Length
    $fileSizeMB = [math]::Round($fileSize / 1MB, 2)
    Write-Host "  Size: $fileSize bytes ($fileSizeMB MB)" -ForegroundColor White
    
    Write-Host ""
    Write-Host "You can now distribute this single file without any dependencies!" -ForegroundColor Green
    Write-Host ""
    Write-Host "To test the executable:" -ForegroundColor Yellow
    Write-Host "  $exePath -help" -ForegroundColor Gray
    Write-Host "  $exePath" -ForegroundColor Gray
    Write-Host ""
} else {
    Write-Host "WARNING: Expected executable not found!" -ForegroundColor Red
    Write-Host "Looking for any caustic_design executables..." -ForegroundColor Yellow
    Get-ChildItem -Recurse -Name "caustic_design*.exe"
}

Write-Host "Press any key to exit..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
