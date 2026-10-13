@echo off
rem Starts the game on Windows: double-click it (or run it from any folder). The game finds its files
rem (assets\, third_party\) from the repository's root, so it is started from there.
setlocal
cd /d "%~dp0"
if not exist "build\rac1-native.exe" (
    echo The game is not built yet: run setup.cmd first.
    pause
    exit /b 1
)
"build\rac1-native.exe" %*
exit /b %errorlevel%
