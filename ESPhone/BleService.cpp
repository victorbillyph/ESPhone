#include "BleService.h"
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>
#include "BleScanner.h"
#include "Config.h"

// ------------------------------------------------------------------
// UUIDs do serviço BLE
// ------------------------------------------------------------------
#define BLE_UUID_SVC     "0000a100-0001-4000-8000-00805f9b34fb"
#define BLE_UUID_CMD     "0000a101-0001-4000-8000-00805f9b34fb"
#define BLE_UUID_STATE   "0000a102-0001-4000-8000-00805f9b34fb"
#define BLE_UUID_NAME    "0000a103-0001-4000-8000-00805f9b34fb"

BleService bleSvc;

bool g_bleRunning = false;
bool g_bleConnected = false;

static AppManager *gApp = nullptr;
static BLEServer  *gServer = nullptr;
static BLECharacteristic *gStateChr = nullptr;
static BLECharacteristic *gNameChr  = nullptr;
static bool gStateDirty = true;

static int  lastIdx   = -1;
static bool lastInApp = false;

// ------------------------------------------------------------------
// Callbacks das características
// ------------------------------------------------------------------
class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *chr) override {
    String val = String(chr->getValue().c_str());
    val.toUpperCase();
    val.trim();

    if (!gApp) return;

    Serial.printf("[BLE] comando recebido: %s\n", val.c_str());
    gStateDirty = true;

    if (val == "NEXT")   gApp->dispatch(NAV_NEXT);
    else if (val == "PREV")   gApp->dispatch(NAV_PREV);
    else if (val == "SELECT") gApp->dispatch(NAV_SELECT);
    else if (val == "BACK")   gApp->dispatch(NAV_BACK);
    else if (val == "HOME")   gApp->dispatch(NAV_HOME);
    else if (val.startsWith("GOTO:")) {
      int i = val.substring(5).toInt();
      gApp->dispatch(NAV_GOTO, i);
    }
    else if (val == "REBOOT") {
      delay(200);
      ESP.restart();
    }
    else {
      Serial.println("[BLE] comando desconhecido");
    }
  }
};

// ------------------------------------------------------------------
// Callbacks da conexão
// ------------------------------------------------------------------
class SrvCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s, esp_ble_gatts_cb_param_t *p) override {
    g_bleConnected = true;
    gStateDirty = true;
    Serial.printf("[BLE] celular conectou (conexão #%u)\n", s->getConnId());
    if (gApp) gApp->notify(1, 150, 150);   // LED: 1 piscada = celular conectou
  }
  void onDisconnect(BLEServer *s) override {
    g_bleConnected = false;
    gStateDirty = true;
    Serial.println("[BLE] celular desconectou");
    if (gApp) gApp->notify(2, 120, 120);   // LED: 2 piscadas = celular desconectou
    BLEDevice::startAdvertising();   // volta a anunciar
  }
};

static void buildAndStartService() {
  BLEDevice::init(Config.get().deviceName.c_str());
  gServer = BLEDevice::createServer();
  gServer->setCallbacks(new SrvCallbacks());

  BLEService *svc = gServer->createService(BLE_UUID_SVC);

  // Comandos (escrita)
  BLECharacteristic *cmdChr = svc->createCharacteristic(
      BLE_UUID_CMD,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
  cmdChr->setCallbacks(new CharCallbacks());
  cmdChr->setValue("");

  // Estado (notify)
  gStateChr = svc->createCharacteristic(
      BLE_UUID_STATE,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  gStateChr->setValue("");

  // Nome do dispositivo (leitura/escrita)
  gNameChr = svc->createCharacteristic(
      BLE_UUID_NAME,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  gNameChr->setValue(Config.get().deviceName.c_str());
  gNameChr->setCallbacks(new CharCallbacks());

  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_UUID_SVC);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  g_bleRunning = true;
  gStateDirty = true;
  Serial.printf("[BLE] anunciando como \"%s\"\n", Config.get().deviceName.c_str());
}

// ------------------------------------------------------------------
// Classe principal
// ------------------------------------------------------------------
void BleService::begin(AppManager *app) {
  gApp = app;
  buildAndStartService();
}

void BleService::reload() {
  if (g_bleRunning) {
    Serial.println("[BLE] reiniciando...");
    BLEDevice::deinit(false);
    delay(50);
  }
  g_bleRunning = false;
  gServer = nullptr;
  buildAndStartService();
  bleScanner.onStackReinit();
}

void BleService::update() {
  if (!gApp || !gServer) return;

  bool changed = (gApp->index() != lastIdx) || (gApp->inApp() != lastInApp);
  if (changed) {
    lastIdx = gApp->index();
    lastInApp = gApp->inApp();
    gStateDirty = true;
  }

  if (!gStateDirty) return;
  gStateDirty = false;

  String st = gApp->stateString() + String(",ip:") +
              (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                             : WiFi.softAPIP().toString());

  gStateChr->setValue(st.c_str());
  if (g_bleConnected) {
    gStateChr->notify();
  }
}

void bleNotifyConfigChanged() {
  bleSvc.reload();
}