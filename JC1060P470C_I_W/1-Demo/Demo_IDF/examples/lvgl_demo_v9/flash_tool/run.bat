@echo off
title ESP Flash Tool
echo Checking Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo.
    echo Python is not installed!
    echo Please install Python from: https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during install!
    echo.
    pause
    exit
)
echo Python OK. Starting Flash Tool...
python "%~dp0flash_ui.py"
if errorlevel 1 (
    echo.
    echo Something went wrong. See error above.
    pause
)
