@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   SimpleOTA Automation Script (Regen + Build)
echo ========================================================



echo [1/2] Generating Web UI Assets...
python lib/SimpleOTA/tools/generate_assets.py
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Asset generation failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [2/2] Compiling and Uploading Firmware...
pio run 
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Upload failed.
    pause
    exit /b %ERRORLEVEL%
)


