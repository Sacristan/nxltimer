/**
 * NXL Field Timer — ESP32 REFEREE (Middle relay)
 *
 * Sits midfield. Connects to field WiFi + relay server.
 * Receives ESP-NOW from GREEN and RED team boxes, forwards to laptop via WebSocket.
 * Broadcasts its WiFi channel every 3s so team boxes stay locked on.
 *
 * GPIO 0  → BOOT button — hold 10s to reset WiFi + open config portal
 *
 * Libraries:
 *   "WebSockets" by Markus Sattler
 *   "WiFiManager" by tzapu
 *   esp_now — built into ESP32 core
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WebSocketsClient.h>
#include <WiFiManager.h>
#include <Preferences.h>

#define AP_NAME       "NXL-Referee"
#define AP_PASSWORD   "speedsoft"
#define PIN_RESET     0
#define SERVER_PORT   82
#define RESET_HOLD_MS 10000

Preferences prefs;
WebSocketsClient ws;
bool wsConnected = false;
char serverIP[40] = "192.168.1.101";

// Broadcast MAC — send channel info to all ESP-NOW devices
uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Queue for ESP-NOW → WebSocket forwarding (can't call WS from callback)
#define QUEUE_SIZE 8
volatile int qHead = 0, qTail = 0;
char cmdQueue[QUEUE_SIZE][32];

void enqueue(const char* cmd){
  int next = (qTail + 1) % QUEUE_SIZE;
  if(next != qHead){ // not full
    strncpy(cmdQueue[qTail], cmd, 31);
    cmdQueue[qTail][31] = 0;
    qTail = next;
  }
}

// ── WiFi portal ───────────────────────────────────────────────────────────────
void startPortal(bool force){
  WiFiManager wm;
  WiFiManagerParameter serverParam("server", "Relay Server IP", serverIP, 40);
  wm.addParameter(&serverParam);
  wm.setConfigPortalTimeout(180);

  if(force){ wm.resetSettings(); wm.startConfigPortal(AP_NAME, AP_PASSWORD); }
  else { wm.autoConnect(AP_NAME, AP_PASSWORD); }

  if(WiFi.status() == WL_CONNECTED){
    strncpy(serverIP, serverParam.getValue(), 40);
    if(strlen(serverIP) > 6){  // only save if something was entered
      prefs.begin("nxl", false);
      prefs.putString("serverIP", serverIP);
      prefs.end();
    }
    Serial.printf("[WiFi] Connected. IP: %s  Channel: %d  Server: %s\n",
      WiFi.localIP().toString().c_str(), WiFi.channel(), serverIP);
  } else {
    Serial.println("[WiFi] Failed — rebooting"); delay(1000); ESP.restart();
  }
}

// ── ESP-NOW receive callback ──────────────────────────────────────────────────
void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  if(len < 1 || len > 31) return;
  char buf[32]; memcpy(buf, data, len); buf[len] = 0;
  Serial.printf("[ESP-NOW] Recv: %s  RSSI: %d\n", buf, info->rx_ctrl->rssi);
  enqueue(buf);
}

void onEspNowSend(const wifi_tx_info_t* info, esp_now_send_status_t status){}

// ── WebSocket ─────────────────────────────────────────────────────────────────
void onWSEvent(WStype_t type, uint8_t* payload, size_t len){
  switch(type){
    case WStype_CONNECTED:
      Serial.printf("[WS] Connected to %s:%d\n", serverIP, SERVER_PORT);
      wsConnected = true; break;
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected"); wsConnected = false; break;
    default: break;
  }
}

// ── Init ESP-NOW ──────────────────────────────────────────────────────────────
void initEspNow(){
  if(esp_now_init() != ESP_OK){
    Serial.println("[ESP-NOW] Init failed — rebooting"); delay(1000); ESP.restart();
  }
  esp_now_register_recv_cb(onEspNowRecv);
  esp_now_register_send_cb(onEspNowSend);

  // Add broadcast peer so we can send channel info to all team boxes
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BROADCAST_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peer);

  Serial.printf("[ESP-NOW] Ready on channel %d\n", WiFi.channel());
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup(){
  Serial.begin(115200); delay(300);
  Serial.println("\n[BOOT] NXL Referee ESP32");
  Serial.printf("[MAC] %s\n", WiFi.macAddress().c_str());

  pinMode(PIN_RESET, INPUT_PULLUP);

  prefs.begin("nxl", true);
  String saved = prefs.getString("serverIP", "");
  prefs.end();
  if(saved.length() > 6) strncpy(serverIP, saved.c_str(), 40);

  startPortal(false);

  // Lock WiFi to fixed channel after connect — critical for ESP-NOW stability
  uint8_t ch = WiFi.channel();
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  Serial.printf("[WiFi] Locked to channel %d\n", ch);

  initEspNow();

  ws.begin(serverIP, SERVER_PORT, "/");
  ws.onEvent(onWSEvent);
  ws.setReconnectInterval(3000);

  Serial.printf("[READY] Referee running. Server: %s:%d\n", serverIP, SERVER_PORT);
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop(){
  ws.loop();
  unsigned long now = millis();

  // Drain command queue → WebSocket
  while(qHead != qTail){
    if(wsConnected) ws.sendTXT(cmdQueue[qHead]);
    else Serial.printf("[WS] Offline — dropped: %s\n", cmdQueue[qHead]);
    qHead = (qHead + 1) % QUEUE_SIZE;
  }

  // Broadcast WiFi channel every 3s so team boxes can stay locked
  static unsigned long lastChanBcast = 0;
  if(now - lastChanBcast > 3000){
    lastChanBcast = now;
    char msg[16];
    snprintf(msg, sizeof(msg), "CH:%d", (int)WiFi.channel());
    esp_now_send(BROADCAST_MAC, (uint8_t*)msg, strlen(msg));
  }

  // BOOT button hold → WiFi reset
  static unsigned long bootPress = 0;
  static bool bootHeld = false, bootDone = false;
  bool bootLow = (digitalRead(PIN_RESET) == LOW);
  if(bootLow){
    if(!bootHeld){ bootHeld = true; bootPress = now; bootDone = false; }
    else if(!bootDone){
      unsigned long held = now - bootPress;
      static unsigned long lastPrint = 0;
      if(now - lastPrint > 1000 && held > 500){
        lastPrint = now;
        Serial.printf("[HOLD] WiFi reset in %d s...\n", (int)((RESET_HOLD_MS-held)/1000));
      }
      if(held >= RESET_HOLD_MS){
        bootDone = true;
        esp_now_deinit();
        startPortal(true);
        uint8_t ch = WiFi.channel();
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        initEspNow();
        ws.begin(serverIP, SERVER_PORT, "/");
      }
    }
  } else { bootHeld = false; bootDone = false; }
}
