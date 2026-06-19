@echo off
title NXL Field Timer — Relay Server
color 0A

echo.
echo  ================================================
echo   NXL PAINTBALL TIMER — Relay Server
echo  ================================================
echo.

:: Check if portable node.exe exists next to this bat file
IF EXIST "%~dp0node.exe" (
    echo  Using portable Node.js...
    "%~dp0node.exe" "%~dp0server.js"
    goto end
)

:: Fall back to system Node if installed
where node >nul 2>&1
IF %ERRORLEVEL% == 0 (
    echo  Using system Node.js...
    node "%~dp0server.js"
    goto end
)

:: Neither found
echo  ERROR: Node.js not found!
echo.
echo  Please do ONE of the following:
echo.
echo  OPTION A - Add portable Node (recommended for USB use):
echo    1. Go to: https://nodejs.org/dist/latest/
echo    2. Download:  node-vXX.X.X-win-x64.zip
echo    3. Open the zip, find node.exe inside
echo    4. Copy node.exe into this same folder
echo    5. Double-click start-server.bat again
echo.
echo  OPTION B - Install Node normally:
echo    https://nodejs.org  (LTS version)
echo    Then double-click start-server.bat again
echo.

:end
pause
