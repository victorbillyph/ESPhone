#ifndef ES_CONFIG_H
#define ES_CONFIG_H

#define ESPhone_VERSION "1.0.0"

#include <Arduino.h>

#define NOTE_MAX_LEN 2000

struct DeviceConfig {
  String  deviceName;   // nome do dispositivo (BLE + web)
  String  theme;        // "auto" | "light" | "dark"
  long    accent;       // cor de destaque 0xRRGGBB
  uint8_t ledBrightness; // 0..255 (brilho do LED embutido)
  bool    ledInvert;    // true se o LED é ativo-em-BAIXO
  bool    ledScanExt;   // true = radar BLE em LED externo (2 pinos)
  uint8_t scanSpeakPosPin; // pino "positivo" do LED externo (0 = indefinido)
  uint8_t scanSpeakNegPin; // pino "negativo" do LED externo (0 = indefinido)
  bool    apEnabled;    // manter Access Point ligado
  bool    apShareInternet; // NAT: compartilhar internet do STA com clientes do AP
  String  apSsid;
  String  apPassword;
  String  staSsid;      // cliente WiFi (opcional)
  String  staPassword;
  // LED externo do radar BLE (2 pinos: positivo e negativo)
  bool    extLedOn;     // LED SCAN externo habilitado
  bool    extLedInvert; // true -> "negativo" recebe o sinal ativo
  uint8_t extLedPosPin; // pino do terminal POSITIVO (0 = não definido)
  uint8_t extLedNegPin; // pino do terminal NEGATIVO (0 = não definido)
  // Busca intermitente (quando LED externo desligado)
  bool    intermittentScan;  // verifica BLE periodicamente se extLedOn=false
  uint16_t intermittentIntervalMs; // intervalo em ms (default 30000)
  // Caça-WiFi (auto-connect livre)
  bool    wifiHunter;       // ativar caça-wifi
  uint16_t hunterInterval;  // intervalo de scan em segundos
  // UI PIN (4 dígitos) - vazio = não configurado
  String  uiPin;
};

class ConfigClass {
public:
  void init();
  DeviceConfig &get() { return cfg_; }

  void save();
  void reset();

  void saveNotes(const String &notes);
  String getNotes();

private:
  void loadDefaults();
  DeviceConfig cfg_;
};

extern ConfigClass Config;

#endif