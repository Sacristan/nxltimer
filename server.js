#!/usr/bin/env node
/**
 * NXL Paintball Timer — WebSocket Relay Server
 *
 * Browser  → ws://LAPTOP_IP:81   (app connects here)
 * ESP32s   → ws://LAPTOP_IP:82   (devices connect here)
 *
 * Any command from any ESP32 is forwarded to the browser.
 * Any command from the browser is forwarded to all ESP32s.
 */

const WebSocket = require('ws');
const os = require('os');

const APP_PORT    = 81;
const DEVICE_PORT = 82;

// ── Get local IP for display ─────────────────────────────────────────────────
function getLocalIP() {
  const ifaces = os.networkInterfaces();
  for (const name of Object.keys(ifaces)) {
    for (const iface of ifaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) return iface.address;
    }
  }
  return '127.0.0.1';
}

const ip = getLocalIP();

// ── Browser server (port 81) ─────────────────────────────────────────────────
let appSocket = null;
const appServer = new WebSocket.Server({ port: APP_PORT });

appServer.on('listening', () => {
  log('APP', `Browser WS ready  →  ws://${ip}:${APP_PORT}`);
});

appServer.on('connection', (ws) => {
  log('APP', 'Browser connected');
  appSocket = ws;

  ws.on('message', (msg) => {
    const cmd = msg.toString().trim();
    log('APP→ESP', cmd);
    for (const esp of espSockets) {
      if (esp.readyState === WebSocket.OPEN) esp.send(cmd);
    }
  });

  ws.on('close', () => {
    log('APP', 'Browser disconnected');
    appSocket = null;
  });

  ws.on('error', () => { appSocket = null; });
});

// ── ESP32 server (port 82) ───────────────────────────────────────────────────
const espSockets = new Set();
const espServer  = new WebSocket.Server({ port: DEVICE_PORT });

espServer.on('listening', () => {
  log('DEVICE', `ESP32 WS ready     →  ws://${ip}:${DEVICE_PORT}`);
  printBanner(ip);
});

espServer.on('connection', (ws, req) => {
  const devIP = req.socket.remoteAddress.replace('::ffff:', '');
  log('DEVICE', `ESP32 connected from ${devIP}`);
  espSockets.add(ws);

  ws.on('message', (msg) => {
    const cmd = msg.toString().trim();
    log('ESP→APP', cmd);
    if (appSocket && appSocket.readyState === WebSocket.OPEN) {
      appSocket.send(cmd);
    } else {
      log('RELAY', 'No browser connected — command queued/dropped');
    }
  });

  ws.on('close', () => {
    log('DEVICE', `ESP32 ${devIP} disconnected`);
    espSockets.delete(ws);
  });

  ws.on('error', () => espSockets.delete(ws));
});

// ── Helpers ──────────────────────────────────────────────────────────────────
function log(tag, msg) {
  const t = new Date().toTimeString().slice(0, 8);
  console.log(`[${t}] [${tag.padEnd(7)}] ${msg}`);
}

function printBanner(ip) {
  console.log('\n' + '═'.repeat(52));
  console.log('  NXL PAINTBALL TIMER — Relay Server');
  console.log('═'.repeat(52));
  console.log(`  Laptop IP   :  ${ip}`);
  console.log(`  Browser     :  Open nxl-timer.html and enter`);
  console.log(`                 this IP in the WS field: ${ip}`);
  console.log(`  ESP32       :  Set SERVER_IP = "${ip}"`);
  console.log(`                 Set SERVER_PORT = ${DEVICE_PORT}`);
  console.log('═'.repeat(52));
  console.log('  Waiting for connections...\n');
}

// Keep alive
process.on('SIGINT', () => { console.log('\nRelay stopped.'); process.exit(0); });
