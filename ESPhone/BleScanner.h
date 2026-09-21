#ifndef ES_BLESCANNER_H
#define ES_BLESCANNER_H

#include <Arduino.h>

class BLEAdvertisedDevice;

#define BLESCAN_MAX_DEV  32
#define BLESCAN_MAX_SVCS 4

struct BleScanEntry {
  char     addr[20];
  char     name[33];
  int8_t   rssi;
  uint32_t lastSeen;
  uint8_t  addrType;
  uint8_t  numServices;
  char     services[BLESCAN_MAX_SVCS][40];
};

struct BleSavedDev {
  char addr[20];
  char name[41];
};

class BleScanner {
public:
  void begin();
  void onStackReinit();      // chamado após o servidor BLE recriar o stack

  bool startScan();
  void stopScan();
  bool isScanning() const { return scanning_; }

  void update();

  String buildScanJson();
  String buildSavedJson();

  bool saveDevice(const String &addr, const String &name);
  bool forgetDevice(const String &addr);
  bool isSaved(const char *addr);

  void onAdvertised(BLEAdvertisedDevice &d); // chamado pelo callback de scan

  // RSSI do dispositivo visto recentemente mais forte (0 se nenhum).
  // Usado pelo modo "radar" do LED.
  int8_t nearestRssi(uint32_t maxAgeMs);

  // ---- Cliente GATT ----
  String gattDiscover(const String &addr);                       // conecta, lista serviços/características
  String gattRead(const String &addr, const String &svc, const String &chr);
  String gattWrite(const String &addr, const String &svc, const String &chr, const String &hex);
  String gattNotify(const String &addr, const String &svc, const String &chr, bool enable);
  String gattStatus();                                           // cliente de notificação: conectado + último valor
  void   disconnectAll();

private:
  BleScanEntry devs_[BLESCAN_MAX_DEV];
  int   nDevs_   = 0;
  bool  scanning_ = false;
  SemaphoreHandle_t mux_ = nullptr;

  void lock();
  void unlock();
  uint8_t addrTypeFor(const char *addr);
};

extern BleScanner bleScanner;

#endif