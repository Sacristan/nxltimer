# Building the Relay Server Executables

Run this ONCE on your own machine. Produces:
- dist/server-win.exe   → Windows field laptop
- dist/server-mac       → Mac field laptop (Intel)
- dist/server-mac-arm   → Mac field laptop (Apple Silicon M1/M2/M3)

## Requirements
- Node.js 18+  →  https://nodejs.org  (free, one-time install)

## Steps

### Windows (run in Command Prompt or PowerShell)
```
cd relay-server
npm install
npx pkg . --targets node18-win-x64,node18-macos-x64,node18-macos-arm64 --output dist/server
```

### Mac/Linux (run in Terminal)
```
cd relay-server
npm install
npx pkg . --targets node18-win-x64,node18-macos-x64,node18-macos-arm64 --output dist/server
```

This takes 1-2 minutes the first time (downloads pkg base binaries).

Output files in dist/:
  server-win.exe     → copy to USB, run on Windows laptops
  server-macos       → copy to USB, run on Mac Intel laptops
  server-macos-arm64 → copy to USB, run on Mac M1/M2/M3 laptops

## On the field laptop

### Windows
Double-click server-win.exe
(If Windows Defender blocks it: right-click → Properties → Unblock, or click "More info → Run anyway")

### Mac
First time only, open Terminal and run:
  chmod +x server-macos        (or server-macos-arm64)
  xattr -d com.apple.quarantine server-macos
Then double-click, or run from Terminal.

The console window shows your laptop's IP address.
Enter that IP in the nxl-timer.html settings screen.
