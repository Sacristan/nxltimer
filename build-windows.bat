@echo off
echo NXL Relay Server Builder
echo ========================
echo Installing dependencies...
call npm install
echo.
echo Building executables...
call npx pkg . --targets node18-win-x64,node18-macos-x64,node18-macos-arm64 --output dist/server
echo.
echo Done! Check the dist/ folder.
echo   server-win.exe       Windows
echo   server-macos         Mac Intel
echo   server-macos-arm64   Mac Apple Silicon
pause
