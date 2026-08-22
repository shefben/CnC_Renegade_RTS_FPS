@echo off
setlocal
if "%~1"=="" (
    echo Drag TSR_GDI_MCV_V2_GMAX_3DSMAX.ms onto this file.
    echo.
    pause
    exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Fix_MCV_V2_for_gmax.ps1" "%~1"
if errorlevel 1 (
    echo.
    echo Patch failed.
    pause
    exit /b 1
)
echo.
echo Patch completed. Run the newly created *_GMAX_FIXED.ms file in gmax.
pause
