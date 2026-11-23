@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo ERROR: Microsoft C++ compiler not found.
  echo Install Visual Studio Build Tools with the "Desktop development with C++" workload,
  echo then run this file from "x64 Native Tools Command Prompt for VS".
  pause
  exit /b 1
)
if not exist build mkdir build
cl /nologo /std:c++17 /O1 /MT /EHsc /W4 /DUNICODE /D_UNICODE PinTop.cpp /link /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /OUT:build\PinTop.exe user32.lib shell32.lib advapi32.lib
if errorlevel 1 exit /b 1
echo.
echo Build complete: %CD%\build\PinTop.exe
echo This is a native Win32 executable; no .NET runtime is required.
pause
