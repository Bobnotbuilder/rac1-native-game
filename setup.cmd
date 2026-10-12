@echo off
rem rac1-native's guided setup on Windows: checks your system, builds the game, checks your disc image
rem and generates the game's data. The work is done by tools\setup.py; the tools it needs (Python, clang, CMake,
rem Ninja) come from a private MSYS2 that tools\windows\toolchain.ps1 downloads into .toolchain\ on the first run.
rem From an MSYS2 CLANG64 / UCRT64 shell, that shell's own tools are used instead (see docs\setup.md).
rem Usage: setup.cmd [--iso <path-to-your-iso>] [--force [steps]] [-j N]  (setup.cmd --help)
rem Double-clicking it works too (it asks for the disc image), and so does dropping the disc image on it.
setlocal
set "DIR=%~dp0"
set "ARGS=%*"
set "WAIT="
rem (double-clicked, or a disc image dropped on it: the window stays open at the end)
if "%~1"=="" set "WAIT=1"
if not "%~1"=="" if exist "%~1" (
    set "ARGS=--iso %*"
    set "WAIT=1"
)

if /i "%MSYSTEM%"=="CLANG64" goto own_tools
if /i "%MSYSTEM%"=="UCRT64" goto own_tools

powershell -NoProfile -ExecutionPolicy Bypass -File "%DIR%tools\windows\toolchain.ps1"
if errorlevel 1 (
    set "RESULT=1"
    goto done
)
set "PATH=%DIR%.toolchain\msys64\clang64\bin;%PATH%"
"%DIR%.toolchain\msys64\clang64\bin\python.exe" "%DIR%tools\setup.py" %ARGS%
set "RESULT=%errorlevel%"
goto done

:own_tools
set "RESULT=1"
where python >nul 2>nul
if errorlevel 1 (
    echo The setup needs Python 3.8 or newer. In this shell:
    echo     pacman -S --needed mingw-w64-clang-x86_64-python
    goto done
)
python "%DIR%tools\setup.py" %ARGS%
set "RESULT=%errorlevel%"

:done
if defined WAIT pause
exit /b %RESULT%
