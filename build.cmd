@echo off
setlocal
cmake -S . -B build -A x64
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release --parallel
if errorlevel 1 exit /b %errorlevel%
echo.
echo Build complete. Keep PinTop.exe and PinTopHook.dll together.
endlocal
