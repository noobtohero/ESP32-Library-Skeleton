@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   SimpleOTA Automation Script (Regen + Upload + Monitor)
echo ========================================================

:: Step 1: Conceptual Ctrl+C 
:: (เมื่อรันสคริปต์นี้ ตัว Serial Monitor เดิมจะถูกปลดออกอยู่แล้วถ้าใช้ใน Terminal เดียวกัน)

echo [1/3] Generating Web UI Assets...
python lib/SimpleOTA/tools/generate_assets.py
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Asset generation failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [2/3] Compiling and Uploading Firmware...
pio run -t upload
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Upload failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Starting Serial Monitor...
echo (Press Ctrl+C to stop the monitor and return to prompt)
echo.
pio device monitor

pause
