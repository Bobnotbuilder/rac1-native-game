@echo off
rem rac1-native's guided setup on Windows: checks your system, builds the game, checks your disc image
rem and generates the game's data. The work is done by tools\setup.py, which needs Python 3.8 or newer
rem (MSYS2's CLANG64 one, see docs\setup.md).
rem Usage: setup.cmd [--iso <path-to-your-iso>] [--force [steps]] [-j N]  (setup.cmd --help)
setlocal
set "DIR=%~dp0"
where python >nul 2>nul
if %errorlevel%==0 (
    python -c "import sys; sys.exit(sys.version_info < (3, 8))" >nul 2>nul
    if not errorlevel 1 (
        python "%DIR%tools\setup.py" %*
        exit /b %errorlevel%
    )
)
where py >nul 2>nul
if %errorlevel%==0 (
    py -3 -c "import sys; sys.exit(sys.version_info < (3, 8))" >nul 2>nul
    if not errorlevel 1 (
        py -3 "%DIR%tools\setup.py" %*
        exit /b %errorlevel%
    )
)
echo The setup needs Python 3.8 or newer. Install MSYS2 (https://www.msys2.org), then in its CLANG64 shell:
echo     pacman -S --needed mingw-w64-clang-x86_64-python
echo and run setup.cmd again from that shell (see docs\setup.md).
exit /b 1
