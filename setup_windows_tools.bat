@echo off
setlocal
title Football Match Simulator - Windows Environment Setup

echo ========================================================
echo   Automated Windows C++ & SFML Development Setup
echo ========================================================
echo.
echo Installing CMake and MSYS2 (MinGW-w64 toolchain) using winget...
echo.

where winget.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] winget is not installed on this system.
    echo Please install MSYS2 manually from: https://www.msys2.org/
    pause
    exit /b 1
)

echo [1/2] Installing Kitware CMake...
winget install --id Kitware.CMake -e --source winget --accept-source-agreements --accept-package-agreements

echo [2/2] Installing MSYS2...
winget install --id MSYS2.MSYS2 -e --source winget --accept-source-agreements --accept-package-agreements

echo.
echo ========================================================
echo   Installation completed!
echo ========================================================
echo After MSYS2 finishes installing:
echo 1. Open 'MSYS2 UCRT64' from your Start Menu.
echo 2. Run:
echo    pacman -Syu --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sfml
echo 3. Add C:\msys64\ucrt64\bin to your System PATH.
echo 4. Then double-click 'run.bat' to build and run!
echo ========================================================
echo.
pause
