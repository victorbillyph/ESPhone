#ifndef ES_WIFISNIFFER_H
#define ES_WIFISNIFFER_H

#include <Arduino.h>
#include "esp_wifi_types.h"

#define SNIFF_MAX_NETS 32
#define SNIFF_MAX_MACS 64

struct SniffNet {
  char     ssid[33];
  uint8_t  channel;
  int8_t   rssi;
  uint32_t beacons;
  bool     secure;
  uint32_t lastSeen;
};

struct SniffMac {
  uint8_t  mac[6];
  uint32_t count;
  int8_t   rssi;
  uint8_t  channel;
  uint8_t  kind;   // 0=mgmt 1=ctrl 2=data
  uint32_t lastSeen;
};

class WifiSniffer {
public:
  void begin();
  bool setEnabled(bool on);
  bool enabled() const { return on_; }
  bool setHop(bool on);
  bool hop() const { return hop_; }
  void update();

  String buildJson();
  void   reset();

private:
  void onPacket(void *buf, wifi_promiscuous_pkt_type_t type);
  void touchNet(const char *ssid, uint8_t len, uint8_t ch, int8_t rssi, bool secure);
  void touchMac(const uint8_t *mac, uint8_t ch, int8_t rssi, uint8_t kind);

  SniffNet nets_[SNIFF_MAX_NETS];
  int      nNets_ = 0;
  SniffMac macs_[SNIFF_MAX_MACS];
  int      nMacs_ = 0;

  volatile uint32_t totalPkt_ = 0, dataPkt_ = 0, mgmtPkt_ = 0, ctrlPkt_ = 0, miscPkt_ = 0;
  volatile uint32_t beaconPkt_ = 0, probePkt_ = 0;
  volatile int16_t  lastRssi_ = 0, rssiMin_ = 0, rssiMax_ = 0;

  uint32_t lastTick_ = 0, lastCount_ = 0;
  uint16_t pps_ = 0;

  bool     on_ = false, hop_ = false;
  uint8_t  hopCh_ = 1;
  uint32_t lastHop_ = 0;
  uint8_t  curCh_ = 0;

  SemaphoreHandle_t mux_ = nullptr;
  void lock();
  void unlock();

  friend void wifiSniffCbDispatch(void *buf, wifi_promiscuous_pkt_type_t type);
};

extern WifiSniffer wifiSniffer;

#endif