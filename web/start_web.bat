@echo off
REM ==========================================================
REM   start_web.bat
REM   Location: web\start_web.bat
REM   NOTE: This file is best viewed/edited as UTF-8 or ANSI.
REM         All echo output is English to avoid code-page issues.
REM ==========================================================
chcp 65001 >nul
setlocal EnableExtensions

echo ============================================
echo   Shanghai Subway Web - One-click Starter
echo ============================================
echo.

REM --- Locate project root (parent of this bat) ---
cd /d "%~dp0.."
set "PROJECT_ROOT=%cd%"
echo [INFO] Project root = %PROJECT_ROOT%
echo.

REM ============= Auto-detect g++.exe =============
set "GXX_DIR="
if exist "D:\Software\mingw64\bin\g++.exe"   set "GXX_DIR=D:\Software\mingw64\bin"
if "%GXX_DIR%"=="" if exist "C:\msys64\mingw64\bin\g++.exe"   set "GXX_DIR=C:\msys64\mingw64\bin"
if "%GXX_DIR%"=="" if exist "C:\mingw64\bin\g++.exe"          set "GXX_DIR=C:\mingw64\bin"
if "%GXX_DIR%"=="" if exist "C:\MinGW\bin\g++.exe"            set "GXX_DIR=C:\MinGW\bin"
if "%GXX_DIR%"=="" if exist "C:\TDM-GCC-64\bin\g++.exe"       set "GXX_DIR=C:\TDM-GCC-64\bin"
if "%GXX_DIR%"=="" where g++ >nul 2>&1 && for /f "delims=" %%i in ('where g++') do if "%GXX_DIR%"=="" for %%j in ("%%i") do set "GXX_DIR=%%~dpj"
if "%GXX_DIR%"=="" (
    echo [ERROR] g++.exe not found in any common location.
    echo         Please install MinGW-w64, or add its bin folder to PATH.
    pause
    exit /b 1
)
set "PATH=%GXX_DIR%;%PATH%"
echo [INFO] g++    = %GXX_DIR%\g++.exe

REM ============= Auto-detect python.exe =============
set "PY_DIR="
if exist "D:\Software\Python\Python313\python.exe" set "PY_DIR=D:\Software\Python\Python313"
if "%PY_DIR%"=="" if exist "D:\Software\Python\Python312\python.exe" set "PY_DIR=D:\Software\Python\Python312"
if "%PY_DIR%"=="" if exist "D:\Software\Python\Python311\python.exe" set "PY_DIR=D:\Software\Python\Python311"
if "%PY_DIR%"=="" if exist "D:\Software\Python\Python310\python.exe" set "PY_DIR=D:\Software\Python\Python310"
if "%PY_DIR%"=="" if exist "C:\Python313\python.exe" set "PY_DIR=C:\Python313"
if "%PY_DIR%"=="" if exist "C:\Python312\python.exe" set "PY_DIR=C:\Python312"
if "%PY_DIR%"=="" if exist "C:\Python311\python.exe" set "PY_DIR=C:\Python311"
if "%PY_DIR%"=="" if exist "C:\Python310\python.exe" set "PY_DIR=C:\Python310"
if "%PY_DIR%"=="" if exist "%LOCALAPPDATA%\Programs\Python\Python313\python.exe" set "PY_DIR=%LOCALAPPDATA%\Programs\Python\Python313"
if "%PY_DIR%"=="" if exist "%LOCALAPPDATA%\Programs\Python\Python312\python.exe" set "PY_DIR=%LOCALAPPDATA%\Programs\Python\Python312"
if "%PY_DIR%"=="" if exist "C:\Users\xunlin\AppData\Roaming\uv\python\cpython-3.12.11-windows-x86_64-none\python.exe" set "PY_DIR=C:\Users\xunlin\AppData\Roaming\uv\python\cpython-3.12.11-windows-x86_64-none"
if "%PY_DIR%"=="" where python >nul 2>&1 && for /f "delims=" %%i in ('where python') do if "%PY_DIR%"=="" for %%j in ("%%i") do set "PY_DIR=%%~dpj"
if "%PY_DIR%"=="" (
    echo [ERROR] python.exe not found in any common location.
    echo         Please install Python 3.7+, or add its folder to PATH.
    pause
    exit /b 1
)
set "PATH=%PY_DIR%;%PATH%"
echo [INFO] python = %PY_DIR%\python.exe
echo.

REM ============= [1/3] Compile C++ Web entry =============
echo [1/3] Compiling main_web.cpp ...
cd /d "%PROJECT_ROOT%\web"
g++ -std=c++11 -O2 -I.. -o main_web.exe main_web.cpp
if errorlevel 1 (
    echo [ERROR] g++ compile failed.
    pause
    exit /b 1
)
echo       [OK] web\main_web.exe built.
echo.

REM ============= [2/3] Check Python dependencies =============
python -c "import fastapi, uvicorn" 2>nul
if errorlevel 1 (
    echo [2/3] Installing Python deps: fastapi, uvicorn ...
    python -m pip install fastapi uvicorn
    if errorlevel 1 (
        echo [ERROR] pip install failed. Try: python -m pip install fastapi uvicorn
        pause
        exit /b 1
    )
)
echo [2/3] Python deps ready.
echo.

REM ============= [3/3] Start Web server =============
echo [3/3] Starting FastAPI server ...
echo       Open http://127.0.0.1:3724 in your browser.
echo       Press Ctrl+C in this window to stop.
echo.
cd /d "%PROJECT_ROOT%\web"
python server.py

echo.
echo [INFO] Server stopped.
pause
exit /b 0
