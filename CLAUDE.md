# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NXL Speedsoft Timer is a distributed field timing system for competitive speedsoft tournaments (NXL/Xball format). It has three distinct components that must work together.

## Running and Building

**Run the relay server (development):**
```
node server.js
```

**Run via launcher scripts:**
- Windows: `start-server.bat`
- Mac: `./start-server-mac.sh`

**Build portable executables (requires `npm install` first):**
```
npm install
npx pkg . --targets node18-win-x64,node18-macos-x64,node18-macos-arm64 --output dist/server
```
Outputs: `dist/server-win.exe`, `dist/server-macos`, `dist/server-macos-arm64`

**Browser UI:** Open `nxl-timer.html` directly in a browser — no build step needed.

**ESP32 firmware:** Flash each `.ino` file with Arduino IDE to the corresponding hardware board.

There is no test suite and no linter configured.

## Architecture

Three-tier distributed system:

```
GREEN TEAM BOX ──ESP-NOW──┐
                           ├──> REFEREE ESP32 ──WebSocket──> RELAY SERVER (port 82)
RED TEAM BOX   ──ESP-NOW──┘                                        │
                                                               port 81
                                                                    │
                                                          BROWSER (nxl-timer.html)
```

### Relay Server (`server.js`)

Single Node.js file using the `ws` library. Runs two WebSocket servers simultaneously:
- **Port 81** — browser clients
- **Port 82** — ESP32 devices

All messages are forwarded bidirectionally between the two pools. No message parsing or game logic lives here.

### Browser UI (`nxl-timer.html`)

Single self-contained HTML file (~1000 lines) with inline CSS and JavaScript. No external dependencies at runtime. Responsibilities:
- All game state, timer logic, scoring, and phase transitions
- WebSocket client connecting to the relay server IP (configurable in settings)
- Audio cue generation via Web Audio API
- Settings persistence via `localStorage`

Game phases: `READY → COUNTDOWN → PLAYING → BREAK → TIMEOUT → GAME OVER`

### ESP32 Firmware (three boards)

| File | Board | Role |
|------|-------|------|
| `esp32_GREEN/esp32_GREEN.ino` | Green team box | GPIO buttons → ESP-NOW broadcasts |
| `esp32_RED/esp32_RED.ino` | Red team box | GPIO buttons → ESP-NOW broadcasts |
| `esp32_REFEREE/esp32_REFEREE.ino` | Referee device | Receives ESP-NOW, relays to relay server via WebSocket |

Button GPIO pins (all boards): GPIO 13 (space), GPIO 27 (main/score), GPIO 12 (surrender), GPIO 0 (BOOT/reinit on 10s hold).

Command strings sent over WebSocket: `GREEN_BTN`, `GREEN_SPACE`, `GREEN_SURRENDER`, `GREEN_ALIVE`, `RED_BTN`, `RED_SPACE`, `RED_SURRENDER`, `RED_ALIVE`.

## Key Constraints

- `nxl-timer.html` must remain a single file — it is deployed by copying one file to a laptop.
- The relay server must be a portable executable for USB deployment with no Node.js installation required on the tournament laptop.
- ESP-NOW channel sync is handled by the REFEREE broadcasting `CH:X` messages; GREEN/RED boxes listen and switch channels accordingly.
- Score button cooldown is 3 seconds (debounce in ESP32 firmware). Button debounce is 200ms.
