/*
#include "FileSystem.h"
#include "AppRegistry.h"
#include "AppApi.h"
 * ============================================================
 *  ESPhone — transforma um ESP32 em um dispositivo móvel
 * ============================================================
 *
 *  - TELA:      interface web responsiva (celular e PC)
 *  - NAVEGAÇÃO: botões na web OU pelo botão BOOT físico
 *               (LED embutido mostra o app selecionado)
 *  - APPS:      Configurações (nome, WiFi, tema, LED),
 *               Sobre, Sistema (diagnóstico) e Notas
 *  - CONEXÃO:   WiFi (Access Point + Cliente) e Bluetooth BLE
 *
 *  Como usar:
 *   1) Carregue este sketch no Arduino IDE (placa "ESP32 Dev Module")
 *   2) O ESP32 cria um WiFi "ESPhone-XXXX" (senha esphone123)
 *   3) Abra http://192.168.4.1 no celular ou PC
 *
 *  Navegação pelo botão BOOT:
 *   - Aperto CURTO: próximo app (LED pisca a posição: 1x, 2x...)
 *   - Segurar (~0,6s): abre o app selecionado / volta
 *
 * ============================================================
 */

#include <Arduino.h>
#include "Config.h"
#include "AppManager.h"
#include "FileSystem.h"
#include "AppRegistry.h"
#include "AppApi.h"
#include "WebPortal.h"
#include "BleService.h"
#include "BleScanner.h"
#include "WifiSniffer.h"

WebPortal portal;

// ---------------------------------------------------------------------------
// Radar BLE do LED: enquanto o LED está suspenso o ESP segue escaneando
// Bluetooth. Se achar algo, o LED pisca conforme a distância (fica constante
// enquanto houver dispositivos). Se não achar nada, pausa 30s para poupar
// bateria.
// ---------------------------------------------------------------------------
static uint32_t ambScanStart = 0;
static uint32_t ambResumeAt  = 0;
static bool     ambOwned     = false;

static uint8_t rssiToLevel(int8_t rssi) {
  if (rssi >= -50) return 4;   // colado  -> aceso
  if (rssi >= -65) return 3;   // perto   -> piscada rápida
  if (rssi >= -78) return 2;   // médio   -> piscada média
  return 1;                    // longe   -> piscada longa
}

static void ambientBleTick() {
  bool bleAppOpen = appMgr.inApp() && appMgr.index() == 1;

  // Com LED externo de scan, o radar roda SEMPRE (ignora a suspensão).
  bool needRadar = appMgr.suspended() || appMgr.scanLedExt();

  if (!needRadar) {                   // acordado s/ LED externo: devolve o scan
    if (ambOwned) { bleScanner.stopScan(); ambOwned = false; }
    return;
  }
  if (bleAppOpen) {                   // app Bluetooth manda no scan
    ambOwned = false;
    return;
  }

  uint32_t now = millis();
  if ((int32_t)(now - ambResumeAt) < 0) return;  // em pausa (nada encontrado)

  if (!ambOwned) {
    if (!bleScanner.isScanning()) bleScanner.startScan();
    ambOwned     = true;
    ambScanStart = now;
  }

  int8_t rssi = bleScanner.nearestRssi(4000);
  if (rssi != 0) {
    appMgr.setAmbient(true, rssiToLevel(rssi));  // achou: fica constante
  } else if (now - ambScanStart > 4000) {
    appMgr.setAmbient(false, 0);                 // nada: pausa por 30s
    bleScanner.stopScan();
    ambOwned    = false;
    ambResumeAt = now + 30000;
  } else {
    appMgr.setAmbient(false, 0);                 // ainda procurando
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("==================================");
  Serial.printf(" ESPhone v%s\n", ESPhone_VERSION);
  Serial.println("==================================");

  Config.init();
  appMgr.begin();
  Fs.begin();               // LittleFS: /system /apps /config /data
  bleSvc.begin(&appMgr);
  bleScanner.begin();
  portal.begin(&appMgr);
  wifiSniffer.begin();
  api.mount(portal.serverRef(), &Fs, &Apps);   // rotas JSON /api/* (mesmo contrato do app nativo)

  Serial.println("Pronto! Acesse http://192.168.4.1 (ou o IP STA)");
  Serial.println("Digite 'help' no monitor serial para configurar o WiFi.");
}

void loop() {
  appMgr.update();       // botão BOOT + LED
  ambientBleTick();      // radar BLE do LED (suspensão)
  bleSvc.update();       // notify de estado BLE
  bleScanner.update();   // scan BLE (quando ativo)
  wifiSniffer.update();  // monitor WiFi (quando ativo)
  portal.update();       // servidor web + WiFi
  portal.handleSerial(); // console serial (config WiFi/AP/nome)
  delay(1);
}