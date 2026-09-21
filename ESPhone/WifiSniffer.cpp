#include "WifiSniffer.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

WifiSniffer wifiSniffer;

static String jescS(const char *s) {
  String out;
  for (const char *p = s; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c < 0x20) { out += ' '; }
    else out += c;
  }
  return out;
}

static void macToStrS(const uint8_t *m, char *out) {
  snprintf(out, 20, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

void WifiSniffer::begin() {
  if (mux_ == nullptr) mux_ = xSemaphoreCreateMutex();

  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb([](void *buf, wifi_promiscuous_pkt_type_t type) {
    wifiSniffer.onPacket(buf, type);
  });
}

void WifiSniffer::lock()   { if (mux_) xSemaphoreTake(mux_, portMAX_DELAY); }
void WifiSniffer::unlock() { if (mux_) xSemaphoreGive(mux_); }

void WifiSniffer::reset() {
  lock();
  nNets_ = 0;
  nMacs_ = 0;
  totalPkt_ = dataPkt_ = mgmtPkt_ = ctrlPkt_ = miscPkt_ = 0;
  beaconPkt_ = probePkt_ = 0;
  lastRssi_ = rssiMin_ = rssiMax_ = 0;
  pps_ = 0;
  lastCount_ = 0;
  unlock();
}

bool WifiSniffer::setEnabled(bool on) {
  if (on == on_) return true;
  if (on) {
    reset();
    if (esp_wifi_set_promiscuous(true) != ESP_OK) return false;
    on_ = true;
    lastTick_ = millis();
    lastHop_ = millis();
  } else {
    esp_wifi_set_promiscuous(false);
    on_ = false;
  }
  return true;
}

bool WifiSniffer::setHop(bool on) {
  hop_ = on;
  if (on) {
    hopCh_ = 1;
    lastHop_ = 0;
  } else if (WiFi.status() == WL_CONNECTED) {
    esp_wifi_set_channel(WiFi.channel(), WIFI_SECOND_CHAN_NONE);
  }
  return true;
}

void WifiSniffer::update() {
  uint32_t now = millis();

  if (on_) {
    if (hop_ && now - lastHop_ >= 1000) {
      lastHop_ = now;
      if (hopCh_ > 13) hopCh_ = 1;
      esp_wifi_set_channel(hopCh_, WIFI_SECOND_CHAN_NONE);
      hopCh_++;
    }
    uint8_t prim;
    wifi_second_chan_t sec;
    if (esp_wifi_get_channel(&prim, &sec) == ESP_OK) curCh_ = prim;

    if (now - lastTick_ >= 1000) {
      uint32_t c = totalPkt_;
      pps_ = (uint16_t)(c - lastCount_);
      lastCount_ = c;
      lastTick_ = now;
    }
  }
}

// ---------------------------------------------------------------------------
void WifiSniffer::touchNet(const char *ssid, uint8_t len, uint8_t ch, int8_t rssi, bool secure) {
  if (len == 0 || len > 32) return;
  char tmp[33];
  memcpy(tmp, ssid, len);
  tmp[len] = 0;

  for (int i = 0; i < nNets_; i++) {
    if (strncmp(nets_[i].ssid, tmp, 32) == 0) {
      if (rssi > nets_[i].rssi) nets_[i].rssi = rssi;
      nets_[i].channel = ch;
      nets_[i].beacons++;
      nets_[i].secure = secure;
      nets_[i].lastSeen = millis();
      return;
    }
  }
  int slot;
  if (nNets_ < SNIFF_MAX_NETS) {
    slot = nNets_++;
  } else {
    slot = 0;
    uint32_t oldest = nets_[0].lastSeen;
    for (int i = 1; i < nNets_; i++) {
      if (nets_[i].lastSeen < oldest) { oldest = nets_[i].lastSeen; slot = i; }
    }
  }
  strncpy(nets_[slot].ssid, tmp, 32);
  nets_[slot].ssid[32] = 0;
  nets_[slot].channel = ch;
  nets_[slot].rssi = rssi;
  nets_[slot].beacons = 1;
  nets_[slot].secure = secure;
  nets_[slot].lastSeen = millis();
}

void WifiSniffer::touchMac(const uint8_t *mac, uint8_t ch, int8_t rssi, uint8_t kind) {
  for (int i = 0; i < nMacs_; i++) {
    if (memcmp(macs_[i].mac, mac, 6) == 0) {
      macs_[i].count++;
      macs_[i].rssi = rssi;
      macs_[i].channel = ch;
      macs_[i].kind = kind;
      macs_[i].lastSeen = millis();
      return;
    }
  }
  int slot;
  if (nMacs_ < SNIFF_MAX_MACS) {
    slot = nMacs_++;
  } else {
    slot = 0;
    uint32_t oldest = macs_[0].lastSeen;
    for (int i = 1; i < nMacs_; i++) {
      if (macs_[i].lastSeen < oldest) { oldest = macs_[i].lastSeen; slot = i; }
    }
  }
  memcpy(macs_[slot].mac, mac, 6);
  macs_[slot].count = 1;
  macs_[slot].rssi = rssi;
  macs_[slot].channel = ch;
  macs_[slot].kind = kind;
  macs_[slot].lastSeen = millis();
}

void WifiSniffer::onPacket(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (!on_) return;
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  if (pkt == nullptr) return;

  int8_t  rssi = (int8_t)pkt->rx_ctrl.rssi;
  uint8_t ch   = (uint8_t)pkt->rx_ctrl.channel;
  uint16_t len = (uint16_t)pkt->rx_ctrl.sig_len;
  const uint8_t *pl = pkt->payload;

  lock();

  totalPkt_++;

  // Pacotes MISC (MIMO/A-MPDU) não têm cabeçalho 802.11 no início do buffer:
  // contabiliza mas não extrai endereço/sinal deles.
  if (type == WIFI_PKT_MISC) { miscPkt_++; unlock(); return; }

  lastRssi_ = rssi;
  if (totalPkt_ == 1) { rssiMin_ = rssi; rssiMax_ = rssi; }
  else {
    if (rssi < rssiMin_) rssiMin_ = rssi;
    if (rssi > rssiMax_) rssiMax_ = rssi;
  }

  uint8_t kind = 2;
  if (type == WIFI_PKT_MGMT) { mgmtPkt_++; kind = 0; }
  else if (type == WIFI_PKT_CTRL) { ctrlPkt_++; kind = 1; }
  else { dataPkt_++; kind = 2; }

  if (len >= 24) {
    uint16_t fc   = pl[0] | (pl[1] << 8);
    uint8_t  ftype = (fc >> 2) & 0x03;
    uint8_t  sub   = (fc >> 4) & 0x0F;

    // MAC de origem (addr2) em frames de gerenciamento/dados
    if (ftype == 0 || ftype == 2) {
      touchMac(pl + 10, ch, rssi, kind);
    }

    if (ftype == 0 && (sub == 8 || sub == 5)) {  // beacon / probe response
      if (sub == 8) beaconPkt_++;
      else probePkt_++;

      bool secure = false;
      if (len >= 36) {
        uint16_t cap = pl[34] | (pl[35] << 8);
        secure = (cap & 0x0010) != 0;
      }
      // percorre as tags a partir do offset 36
      int off = 36;
      while (off + 2 <= len) {
        uint8_t tag = pl[off];
        uint8_t tlen = pl[off + 1];
        if (off + 2 + tlen > len) break;
        if (tag == 0) {
          touchNet((const char *)(pl + off + 2), tlen, ch, rssi, secure);
          break;
        }
        off += 2 + tlen;
      }
    }
  }

  unlock();
}

// ---------------------------------------------------------------------------
String WifiSniffer::buildJson() {
  String out = "{";
  lock();
  out += "\"on\":" + String(on_ ? "true" : "false");
  out += ",\"hop\":" + String(hop_ ? "true" : "false");
  out += ",\"ch\":" + String(curCh_);
  out += ",\"total\":" + String(totalPkt_);
  out += ",\"data\":" + String(dataPkt_);
  out += ",\"mgmt\":" + String(mgmtPkt_);
  out += ",\"ctrl\":" + String(ctrlPkt_);
  out += ",\"misc\":" + String(miscPkt_);
  out += ",\"beacon\":" + String(beaconPkt_);
  out += ",\"probe\":" + String(probePkt_);
  out += ",\"pps\":" + String(pps_);
  out += ",\"rssi\":" + String(lastRssi_);
  out += ",\"rssiMin\":" + String(rssiMin_);
  out += ",\"rssiMax\":" + String(rssiMax_);

  out += ",\"nets\":[";
  bool first = true;
  uint32_t now = millis();
  for (int i = 0; i < nNets_; i++) {
    if (now - nets_[i].lastSeen > 60000) continue;
    if (!first) out += ",";
    first = false;
    out += "{\"ssid\":\"" + jescS(nets_[i].ssid) + "\"";
    out += ",\"ch\":" + String(nets_[i].channel);
    out += ",\"rssi\":" + String(nets_[i].rssi);
    out += ",\"beacons\":" + String(nets_[i].beacons);
    out += ",\"secure\":" + String(nets_[i].secure ? "true" : "false");
    out += "}";
  }
  out += "]";

  out += ",\"macs\":[";
  first = true;
  for (int i = 0; i < nMacs_; i++) {
    if (!first) out += ",";
    first = false;
    char m[20];
    macToStrS(macs_[i].mac, m);
    out += "{\"mac\":\"" + String(m) + "\"";
    out += ",\"count\":" + String(macs_[i].count);
    out += ",\"rssi\":" + String(macs_[i].rssi);
    out += ",\"ch\":" + String(macs_[i].channel);
    out += ",\"kind\":" + String(macs_[i].kind);
    out += "}";
  }
  out += "]";
  unlock();

  out += "}";
  return out;
}