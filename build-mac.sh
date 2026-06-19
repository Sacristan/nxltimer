#!/bin/bash
echo "NXL Relay Server Builder"
echo "========================"
echo "Installing dependencies..."
npm install
echo ""
echo "Building executables..."
npx pkg . --targets node18-win-x64,node18-macos-x64,node18-macos-arm64 --output dist/server
echo ""
echo "Done! Check the dist/ folder."
echo "  dist/server-win.exe       Windows"
echo "  dist/server-macos         Mac Intel"
echo "  dist/server-macos-arm64   Mac Apple Silicon"
