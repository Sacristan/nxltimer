/**
 * NXL Field Timer — ESP32 RED Button Box
 *
 * No WiFi — ESP-NOW only to Referee ESP32.
 * Automatically syncs to referee's WiFi channel via broadcast.
 *
 * GPIO 13 → SHORT press: RED_SPACE
 * GPIO 27 → RED_BTN   (point GREEN / red T/O)
 * GPIO 12 → RED_SURRENDER
 * GPIO  0 → BOOT button (hold 10s to reinit)
 *
 * Libraries: esp_now (built-in)
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

uint8_t REFEREE_MAC[] = {0xF4, 0x2D, 0xC9, 0x86, 0xD5, 0x04};

#define PIN_SPACE      13
#define PIN_BTN_A      27
#define PIN_BTN_B      12
#define PIN_RESET      0
#define DEBOUNCE_MS    200
#define SCORE_COOLDOWN 3000
#define RESET_HOLD_MS  10000

unsigned long scoreCooldownUntil = 0;
int currentChannel = 1;
bool peerReady = false;

void addRefereePeer(int channel){
  esp_now_del_peer(REFEREE_MAC);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, REFEREE_MAC, 6);
  peer.channel = channel;
  peer.encrypt = false;
  peer.ifidx = WIFI_IF_STA;
  if(esp_now_add_peer(&peer) == ESP_OK){
    peerReady = true;
    Serial.printf("[ESP-NOW] Peer added on channel %d\n", channel);
  }
}

void sendCmd(const char* cmd){
  if(!peerReady) return;
  esp_err_t r = esp_now_send(REFEREE_MAC, (uint8_t*)cmd, strlen(cmd));
  Serial.printf("[SEND] %s: %s\n", cmd, r==ESP_OK?"OK":"FAIL");
}

void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len){
  char buf[32]; memcpy(buf, data, len); buf[len]=0;
  if(strncmp(buf, "CH:", 3)==0){
    int ch = atoi(buf+3);
    if(ch > 0 && ch != currentChannel){
      Serial.printf("[CHAN] Switching to channel %d\n", ch);
      currentChannel = ch;
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      addRefereePeer(ch);
    }
  }
}

void onSend(const wifi_tx_info_t* info, esp_now_send_status_t status){
  if(status != ESP_NOW_SEND_SUCCESS)
    Serial.println("[SEND] Delivery failed — will retry next press");
}

void initEspNow(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);

  if(esp_now_init() != ESP_OK){
    Serial.println("[ESP-NOW] Init failed — rebooting");
    delay(2000); ESP.restart();
  }
  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSend);
  addRefereePeer(currentChannel);
  Serial.printf("[ESP-NOW] Ready on channel %d\n", currentChannel);
}

void setup(){
  Serial.begin(115200); delay(200);
  Serial.printf("[MAC] RED: %s\n", WiFi.macAddress().c_str());
  pinMode(PIN_SPACE, INPUT_PULLUP);
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  initEspNow();
  Serial.println("[READY] RED ESP32");
}

void loop(){
  unsigned long now = millis();

  // Heartbeat every 4s
  static unsigned long lastHB = 0;
  if(now - lastHB > 4000){ lastHB = now; sendCmd("RED_ALIVE"); }

  // BOOT hold → reinit
  static unsigned long bootPress=0; static bool bootHeld=false,bootDone=false;
  bool bootLow=(digitalRead(PIN_RESET)==LOW);
  if(bootLow){
    if(!bootHeld){bootHeld=true;bootPress=now;bootDone=false;}
    else if(!bootDone){
      unsigned long held=now-bootPress;
      static unsigned long lp=0;
      if(now-lp>1000&&held>500){lp=now;Serial.printf("[HOLD] Reinit in %ds\n",(int)((RESET_HOLD_MS-held)/1000));}
      if(held>=RESET_HOLD_MS){bootDone=true;esp_now_deinit();initEspNow();}
    }
  } else {bootHeld=false;bootDone=false;}

  // GPIO 13 space — on release
  static bool spHeld=false; static unsigned long spTime=0;
  bool spLow=(digitalRead(PIN_SPACE)==LOW);
  if(spLow&&!spHeld){spHeld=true;spTime=now;}
  else if(!spLow&&spHeld){
    spHeld=false;
    if(now-spTime>DEBOUNCE_MS) sendCmd("RED_SPACE");
  }

  // GPIO 27 & 12 with cooldown
  bool cool=(now<scoreCooldownUntil);
  struct{int pin;const char*cmd;}btns[]={{PIN_BTN_A,"RED_BTN"},{PIN_BTN_B,"RED_SURRENDER"}};
  static unsigned long lp2[2]={0,0};
  for(int i=0;i<2;i++){
    if(digitalRead(btns[i].pin)==LOW&&now-lp2[i]>DEBOUNCE_MS&&!cool){
      lp2[i]=now; scoreCooldownUntil=now+SCORE_COOLDOWN;
      sendCmd(btns[i].cmd);
    }
  }
}
