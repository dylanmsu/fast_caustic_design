@echo off
REM Build script for creating a single, self-contained executable
REM This script builds the caustic_design application as a standalone executable

echo ========================================
echo Building Caustic Design Single Executable
echo ========================================
echo.

REM Check if build directory exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Setup Visual Studio environment
set VSPATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat"
if not exist %VSPATH% (
    echo ERROR: Visual Studio Build Tools not found!
    echo Please ensure Visual Studio 2019 Build Tools are installed.
    pause
    exit /b 1
)

REM Configure with single executable option
echo Configuring CMake for single executable build...
call %VSPATH% && cmake .. -DBUILD_SINGLE_EXECUTABLE=ON -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building single executable...
echo This may take a few minutes...

REM Build the project
call %VSPATH% && cmake --build . --config Release --target caustic_design

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Complete!
echo ========================================

REM Show the result
if exist "Release\caustic_design_standalone.exe" (
    echo Single executable created successfully:
    echo   Location: build\Release\caustic_design_standalone.exe
    
    REM Get file size
    for %%A in ("Release\caustic_design_standalone.exe") do (
        echo   Size: %%~zA bytes
    )
    
    echo.
    echo You can now distribute this single file without any dependencies!
    echo.
    echo To test the executable:
    echo   Release\caustic_design_standalone.exe -help
    echo   Release\caustic_design_standalone.exe
    echo.
) else (
    echo WARNING: Expected executable not found!
    echo Looking for any caustic_design executables...
    dir /s caustic_design*.exe
)

echo Press any key to exit...
pause >nul
