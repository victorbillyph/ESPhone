#ifndef ES_APP_H
#define ES_APP_H

#include <Arduino.h>
#include "Config.h"

#define APP_COUNT 8
extern const char *const APP_NAMES[APP_COUNT];

// Ações de navegação (mesmas usadas pelo botão, web e BLE)
enum NavAction {
  NAV_PREV   = 0,
  NAV_NEXT   = 1,
  NAV_SELECT = 2,
  NAV_BACK   = 3,
  NAV_HOME   = 4,
  NAV_GOTO   = 5
};

// Modo de busca BLE
enum SearchMode {
  SEARCH_OFF = 0,
  SEARCH_CONTINUOUS = 1,   // modo busca contínua (botão boot segurado)
  SEARCH_SINGLE = 2        // busca única (clique curto)
};

// Estados do LED embutido
enum LedMode {
  LED_SOLID_OFF = 0,
  LED_SOLID_ON  = 1,
  LED_SLOW      = 2,   // pisca lento (dentro de um app)
  LED_FAST      = 3,   // pisca rápido (conectando WiFi)
  LED_NBLINK    = 4,   // pisca N vezes (navegação)
  LED_BUSY      = 5,   // double-blink: ESP32 ocupado/trabalhando
  LED_SEARCH    = 6    // modo busca: pisca na distância do dispositivo mais próximo
};

class AppManager {
public:
  void begin();
  void update();

  void dispatch(int action, int param = 0);

  int   index() const  { return idx_; }
  bool  inApp() const  { return inApp_; }
  const char *currentAppName() const;

  // Informa o estado atual em formato texto curto (usado pelo BLE)
  String stateString() const;
  const char *ledPatternName() const;

  // Notificação pontual (sobrepõe o padrão por alguns instantes)
  void notify(int times, uint16_t onMs = 120, uint16_t offMs = 120);

  // Modo "radar" BLE: 1=longe 2=médio 3=perto 4=colado(aceso)
  void setAmbient(bool active, uint8_t level);

  // LED "ocupado" (double-blink): ESP32 trabalhando (BLE scan, WiFi, etc.)
  void setBusy(bool on);

  // Modo de busca BLE
  void setSearchMode(SearchMode mode);
  SearchMode getSearchMode() const { return searchMode_; }

  // Modo setup (primeiro boot): LED pisca lento indicando que o
  // dispositivo aguarda configuração pelo app (nome, WiFi, PIN).
  void setSetupMode(bool on);
  bool setupMode() const { return setupMode_; }

  // "Fala" redes WiFi (pisca LED para cada rede)
  void speakWiFiNetworks();

  // Caça-WiFi
  void triggerWifiHunterScan();
  void updateWifiHunter();
  void playMorse(const uint16_t *pattern, uint8_t len);

  // LED externo de scan BLE (2 pinos). Se habilitado, o radar ignora a
  // suspensão e sempre reflete o radar BLE nesses pinos.
  void setScanLedPins(uint8_t posPin, uint8_t negPin);
  bool scanLedExt() const { return scanLedExt_; }
  bool suspended() const { return suspended_; }

void refreshFromConfig();   // aplica brilho/inversão salvos
  void ledConnecting(bool on); // LED rápido durante conexão WiFi
  void ledReady();             // LED aceso fixo (sem piscar)

  // Self-test: testa LED embutido e LED externo (se configurado)
  String selfTest();

  // GPIO control (para app GPIO): retorna estado dos pinos, bloqueia pinos do sistema
  String gpioGetAll();
  bool   gpioSet(uint8_t pin, bool on);
  uint32_t systemUsedPins() const;  // bitmask dos pinos reservados pelo sistema

 private:
  void handleButton();
  void updateLed();
  void writeLed(bool on);
  void writeScanLed(bool on);   // LED externo (se habilitado)
  void blink(int times, LedMode next);
  void selectApp();

  int   idx_   = 0;
  bool  inApp_ = false;

  // --- suspensão / notificações / radar ---
  uint32_t lastPhys_ = 0;
  bool     suspended_ = false;
  int      oneshotLeft_ = 0;
  uint16_t oneshotOn_ = 120, oneshotOff_ = 120;
  uint32_t oneshotStart_ = 0;
  bool     ambient_ = false;
  uint8_t  ambientLevel_ = 0;

  // --- modo de busca BLE ---
  SearchMode searchMode_ = SEARCH_OFF;
  uint32_t searchStart_ = 0;
  bool     searchSpeakDone_ = false;

  // --- modo setup (primeiro boot) ---
  bool setupMode_ = false;

  // --- busca intermitente (quando LED externo desligado) ---
  uint32_t intermittentLastScan_ = 0;
  bool     intermittentCooldown_ = false;
  uint32_t intermittentCooldownUntil_ = 0;

  // fila de notificações (toca uma após a outra, sem bloquear)
  struct NotifyItem { int times; uint16_t onMs; uint16_t offMs; };
  static const int NOTIFY_QMAX = 6;
  NotifyItem notifyQ_[NOTIFY_QMAX];
  int        notifyN_ = 0;

  // --- LED ---
  LedMode    mode_    = LED_SOLID_ON;
  LedMode    nextMode = LED_SOLID_ON;
  int        blinkLeft = 0;
  uint32_t   blinkStart = 0;
  uint8_t    brightness = 255;
  bool       invert = false;
  uint32_t   busyUntil_ = 0;

  // --- LED externo de scan BLE ---
  bool     scanLedExt_ = false;
  uint32_t scanTone_hz = 2200;  // frequencia do beep do alto-falante de scan
  uint8_t  scanLedPos_ = 0;
  uint8_t  scanLedNeg_ = 0;

  // --- Caça-WiFi ---
  bool     hunterEnabled_ = false;
  uint16_t hunterInterval_ = 30;      // segundos
  uint32_t hunterLastScan_ = 0;
  bool     hunterFound_ = false;
  uint32_t hunterFoundTime_ = 0;
  String   hunterFoundSsid_;
  int8_t   hunterFoundRssi_ = 0;
  enum HunterState { HUNTER_IDLE, HUNTER_SCANNING, HUNTER_WAITING, HUNTER_CONNECTING, HUNTER_FAILED };
  HunterState hunterState_ = HUNTER_IDLE;
  bool     scanningInProgress_ = false;
  uint32_t scanStartTime_ = 0;

  // --- botão ---
  bool       stablePressed = false;
  bool       lastReading   = false;
  bool       prevPressed   = false;
  bool       longFired     = false;
  uint32_t   lastDebounce  = 0;
  uint32_t   pressStart    = 0;
};

extern AppManager appMgr;

#endif
