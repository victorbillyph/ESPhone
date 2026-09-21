#include "BleScanner.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <Preferences.h>

BleScanner bleScanner;

static const char *NVS_NS   = "esphone";
static const char *NVS_KEY  = "blesaved";

// --- utilitários locais ---
static String jesc(const char *s) {
  String out;
  for (const char *p = s; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c < 0x20) { out += ' '; }
    else out += c;
  }
  return out;
}

static void macToStr(const uint8_t *m, char *out) {
  snprintf(out, 20, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

static void toHexStr(char *out, size_t outsz, const uint8_t *data, size_t len) {
  size_t n = 0;
  for (size_t i = 0; i < len && n < outsz - 3; i++) {
    n += snprintf(out + n, outsz - n, "%02X", data[i]);
  }
  out[n] = 0;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static String toAscii(const uint8_t *data, size_t len) {
  String out;
  for (size_t i = 0; i < len && i < 64; i++) {
    char c = (char)data[i];
    out += (c >= 0x20 && c < 0x7F) ? c : '.';
  }
  return out;
}

// ---------------------------------------------------------------------------
//  Callback de resultados de scan
// ---------------------------------------------------------------------------
class BleScanCb : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    bleScanner.onAdvertised(advertisedDevice);
  }
};

// ---------------------------------------------------------------------------
//  Cliente de notificação (mantido conectado enquanto assinado)
// ---------------------------------------------------------------------------
static BLEClient *g_nfClient   = nullptr;
static char       g_nfAddr[20] = "";
static char       g_nfKey[96]  = "";
static char       g_nfHex[512] = "";
static char       g_nfAscii[65];
static uint32_t   g_nfTime     = 0;
static bool       g_nfOn       = false;
static bool       g_nfHasValue = false;

static void nfStore(const String &key, const uint8_t *data, size_t len) {
  strncpy(g_nfKey, key.c_str(), sizeof(g_nfKey) - 1);
  toHexStr(g_nfHex, sizeof(g_nfHex), data, len);
  String a = toAscii(data, len);
  strncpy(g_nfAscii, a.c_str(), sizeof(g_nfAscii) - 1);
  g_nfTime     = millis();
  g_nfHasValue = true;
}

void BleScanner::begin() {
  if (mux_ == nullptr) mux_ = xSemaphoreCreateMutex();
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(160);
  scan->setWindow(100);
  scan->setAdvertisedDeviceCallbacks(new BleScanCb(), true);
}

void BleScanner::onStackReinit() {
  scanning_ = false;
  g_nfOn = false;
  begin();
}

void BleScanner::lock()   { if (mux_) xSemaphoreTake(mux_, portMAX_DELAY); }
void BleScanner::unlock() { if (mux_) xSemaphoreGive(mux_); }

// ---------------------------------------------------------------------------
bool BleScanner::startScan() {
  if (scanning_) return true;
  BLEScan *scan = BLEDevice::getScan();
  if (scan == nullptr) return false;
  if (scan->start(0, nullptr, false)) {
    scanning_ = true;
    return true;
  }
  return false;
}

void BleScanner::stopScan() {
  if (scanning_) {
    BLEDevice::getScan()->stop();
    scanning_ = false;
  }
}

void BleScanner::update() {
}

// Dispositivo visto recentemente com sinal mais forte (aprox. distância)
int8_t BleScanner::nearestRssi(uint32_t maxAgeMs) {
  int8_t best = 0;
  uint32_t now = millis();
  lock();
  for (int i = 0; i < nDevs_; i++) {
    if (devs_[i].lastSeen == 0) continue;
    if (now - devs_[i].lastSeen > maxAgeMs) continue;
    if (best == 0 || devs_[i].rssi > best) best = devs_[i].rssi;
  }
  unlock();
  return best;
}

// ---------------------------------------------------------------------------
void BleScanner::onAdvertised(BLEAdvertisedDevice &d) {
  String name = d.getName();
  String addr = d.getAddress().toString();
  if (addr.length() != 17) return;

  int8_t  rssi = (int8_t)d.getRSSI();
  uint8_t at   = (uint8_t)d.getAddressType();
  char    svec[BLESCAN_MAX_SVCS][40];
  int     svcN = d.getServiceUUIDCount();
  if (svcN > BLESCAN_MAX_SVCS) svcN = BLESCAN_MAX_SVCS;
  for (int i = 0; i < svcN; i++) {
    String su = d.getServiceUUID(i).toString();
    strncpy(svec[i], su.c_str(), 39);
    svec[i][39] = 0;
  }

  lock();

  int idx = -1;
  for (int i = 0; i < nDevs_; i++) {
    if (strcmp(devs_[i].addr, addr.c_str()) == 0) { idx = i; break; }
  }
  if (idx < 0) {
    if (nDevs_ >= BLESCAN_MAX_DEV) { unlock(); return; }
    idx = nDevs_++;
    strncpy(devs_[idx].addr, addr.c_str(), 19);
    devs_[idx].addr[19] = 0;
    devs_[idx].name[0] = 0;
    devs_[idx].numServices = 0;
  }

  devs_[idx].rssi     = rssi;
  devs_[idx].addrType = at;
  devs_[idx].lastSeen = millis();
  if (name.length() > 0) {
    strncpy(devs_[idx].name, name.c_str(), 32);
    devs_[idx].name[32] = 0;
  }
  if (svcN > 0) {
    devs_[idx].numServices = (uint8_t)svcN;
    for (int i = 0; i < svcN; i++) {
      strncpy(devs_[idx].services[i], svec[i], 39);
      devs_[idx].services[i][39] = 0;
    }
  }

  unlock();
}

// ---------------------------------------------------------------------------
String BleScanner::buildScanJson() {
  String out = "{\"scanning\":" + String(scanning_ ? "true" : "false");
  out += ",\"devices\":[";
  lock();
  bool first = true;
  uint32_t now = millis();
  for (int i = 0; i < nDevs_; i++) {
    BleScanEntry &e = devs_[i];
    if (e.lastSeen == 0) continue;
    if (now - e.lastSeen > 20000) continue;
    if (!first) out += ",";
    first = false;
    out += "{\"addr\":\"" + jesc(e.addr) + "\"";
    out += ",\"name\":\"" + jesc(e.name) + "\"";
    out += ",\"rssi\":" + String(e.rssi);
    out += ",\"type\":" + String(e.addrType);
    out += ",\"saved\":" + String(isSaved(e.addr) ? "true" : "false");
    out += ",\"svc\":[";
    for (int k = 0; k < e.numServices; k++) {
      if (k) out += ",";
      out += "\"" + jesc(e.services[k]) + "\"";
    }
    out += "]}";
  }
  unlock();
  out += "]}";
  return out;
}

// ---------------- Dispositivos salvos (NVS) ----------------
static void sanitizeName(const String &in, char *out, size_t sz) {
  size_t k = 0;
  for (size_t i = 0; i < in.length() && k < sz - 1; i++) {
    char c = in[i];
    if (c == ';' || c == '|' || c == '\n' || c == '\r') c = ' ';
    out[k++] = c;
  }
  out[k] = 0;
}

String BleScanner::buildSavedJson() {
  Preferences p;
  p.begin(NVS_NS, true);
  String raw = p.getString(NVS_KEY, "");
  p.end();
  String out = "{\"devices\":[";
  bool first = true;
  int start = 0;
  while (start < (int)raw.length()) {
    int sep = raw.indexOf('|', start);
    if (sep < 0) sep = raw.length();
    String item = raw.substring(start, sep);
    int semi = item.indexOf(';');
    String addr = semi >= 0 ? item.substring(0, semi) : item;
    String name = semi >= 0 ? item.substring(semi + 1) : "";
    if (addr.length() == 17) {
      if (!first) out += ",";
      first = false;
      out += "{\"addr\":\"" + jesc(addr.c_str()) + "\",\"name\":\"" + jesc(name.c_str()) + "\"}";
    }
    start = sep + 1;
  }
  out += "]}";
  return out;
}

bool BleScanner::isSaved(const char *addr) {
  Preferences p;
  p.begin(NVS_NS, true);
  String raw = p.getString(NVS_KEY, "");
  p.end();
  bool found = false;
  int start = 0;
  while (start < (int)raw.length()) {
    int sep = raw.indexOf('|', start);
    if (sep < 0) sep = raw.length();
    String item = raw.substring(start, sep);
    if (item.indexOf(addr) == 0) { found = true; break; }
    start = sep + 1;
  }
  return found;
}

static String savedWrite(const char *addr, const char *name) {
  Preferences p;
  p.begin(NVS_NS, false);
  String raw = p.getString(NVS_KEY, "");
  // remove entrada existente com mesmo endereço
  String out;
  int start = 0;
  while (start < (int)raw.length()) {
    int sep = raw.indexOf('|', start);
    if (sep < 0) sep = raw.length();
    String item = raw.substring(start, sep);
    if (item.indexOf(addr) != 0) { if (out.length()) out += '|'; out += item; }
    start = sep + 1;
  }
  if (out.length()) out += '|';
  out += String(addr) + ";" + String(name);
  p.putString(NVS_KEY, out);
  p.end();
  return out;
}

static void savedRemove(const char *addr) {
  Preferences p;
  p.begin(NVS_NS, false);
  String raw = p.getString(NVS_KEY, "");
  String out;
  int start = 0;
  while (start < (int)raw.length()) {
    int sep = raw.indexOf('|', start);
    if (sep < 0) sep = raw.length();
    String item = raw.substring(start, sep);
    if (item.indexOf(addr) != 0) { if (out.length()) out += '|'; out += item; }
    start = sep + 1;
  }
  p.putString(NVS_KEY, out);
  p.end();
}

bool BleScanner::saveDevice(const String &addr, const String &name) {
  if (addr.length() != 17) return false;
  char nm[42];
  sanitizeName(name, nm, sizeof(nm));
  savedWrite(addr.c_str(), nm);
  return true;
}

bool BleScanner::forgetDevice(const String &addr) {
  savedRemove(addr.c_str());
  return true;
}

// ---------------- Cliente GATT ----------------
uint8_t BleScanner::addrTypeFor(const char *addr) {
  uint8_t type = 0;
  lock();
  for (int i = 0; i < nDevs_; i++) {
    if (strcmp(devs_[i].addr, addr) == 0) { type = devs_[i].addrType; break; }
  }
  unlock();
  return type;
}

String BleScanner::gattDiscover(const String &addr) {
  if (addr.length() != 17) return "{\"error\":\"addr\"}";
  stopScan();

  String out = "{\"addr\":\"" + jesc(addr.c_str()) + "\",\"services\":[";

  BLEClient *cl = BLEDevice::createClient();
  if (cl == nullptr) return "{\"error\":\"client\"}";
  if (!cl->connect(BLEAddress(addr, addrTypeFor(addr.c_str())), 0xFF, 8000)) {
    cl->disconnect();
    return "{\"error\":\"connect\"}";
  }

  std::map<std::string, BLERemoteService *> *svcs = cl->getServices();
  bool firstS = true;
  int sCount = 0, cCount = 0;
  if (svcs != nullptr) {
    for (auto &sp : *svcs) {
      if (sCount++ >= 24) break;
      BLERemoteService *sv = sp.second;
      if (!firstS) out += ",";
      firstS = false;
      out += "{\"uuid\":\"" + jesc(sv->getUUID().toString().c_str()) + "\",\"chars\":[";
      std::map<std::string, BLERemoteCharacteristic *> *chrs = sv->getCharacteristics();
      bool firstC = true;
      if (chrs != nullptr) {
        for (auto &cp : *chrs) {
          if (cCount++ >= 64) break;
          BLERemoteCharacteristic *ch = cp.second;
          if (!firstC) out += ",";
          firstC = false;
          out += "{\"uuid\":\"" + jesc(ch->getUUID().toString().c_str()) + "\"";
          out += ",\"read\":" + String(ch->canRead() ? "true" : "false");
          out += ",\"write\":" + String(ch->canWrite() ? "true" : "false");
          out += ",\"noResp\":" + String(ch->canWriteNoResponse() ? "true" : "false");
          out += ",\"notify\":" + String(ch->canNotify() ? "true" : "false");
          out += ",\"indicate\":" + String(ch->canIndicate() ? "true" : "false");
          out += "}";
        }
      }
      out += "]}";
    }
  }
  out += "]}";

  cl->disconnect();
  return out;
}

String BleScanner::gattRead(const String &addr, const String &svc, const String &chr) {
  if (addr.length() != 17) return "{\"error\":\"addr\"}";
  stopScan();
  String out = "{\"addr\":\"" + jesc(addr.c_str()) + "\"";
  BLEClient *cl = BLEDevice::createClient();
  if (cl == nullptr) return "{\"error\":\"client\"}";
  if (!cl->connect(BLEAddress(addr, addrTypeFor(addr.c_str())), 0xFF, 6000)) {
    cl->disconnect();
    return "{\"error\":\"connect\"}";
  }
  BLERemoteService *sv = cl->getService(BLEUUID(svc.c_str()));
  if (sv == nullptr) { cl->disconnect(); return "{\"error\":\"service\"}"; }
  BLERemoteCharacteristic *ch = sv->getCharacteristic(BLEUUID(chr.c_str()));
  if (ch == nullptr) { cl->disconnect(); return "{\"error\":\"char\"}"; }
  String val = ch->readValue();
  char hex[512];
  toHexStr(hex, sizeof(hex), (const uint8_t *)val.c_str(), val.length());
  String ascii = toAscii((const uint8_t *)val.c_str(), val.length());
  out += ",\"hex\":\"" + String(hex) + "\",\"ascii\":\"" + jesc(ascii.c_str()) + "\",\"size\":" + String(val.length());
  out += "}";
  cl->disconnect();
  return out;
}

String BleScanner::gattWrite(const String &addr, const String &svc, const String &chr, const String &hex) {
  if (addr.length() != 17) return "{\"error\":\"addr\"}";
  stopScan();
  String out = "{\"addr\":\"" + jesc(addr.c_str()) + "\"";
  BLEClient *cl = BLEDevice::createClient();
  if (cl == nullptr) return "{\"error\":\"client\"}";
  if (!cl->connect(BLEAddress(addr, addrTypeFor(addr.c_str())), 0xFF, 6000)) {
    cl->disconnect();
    return "{\"error\":\"connect\"}";
  }
  BLERemoteService *sv = cl->getService(BLEUUID(svc.c_str()));
  if (sv == nullptr) { cl->disconnect(); return "{\"error\":\"service\"}"; }
  BLERemoteCharacteristic *ch = sv->getCharacteristic(BLEUUID(chr.c_str()));
  if (ch == nullptr) { cl->disconnect(); return "{\"error\":\"char\"}"; }

  uint8_t buf[256];
  size_t len = 0;
  bool okHex = true;
  int hi = -1;
  for (size_t i = 0; i < hex.length() && len < sizeof(buf); i++) {
    char c = hex[i];
    if (c == ' ' || c == ':' || c == '-') continue;
    int h = hexNibble(c);
    if (h < 0) { okHex = false; break; }
    if (hi < 0) hi = h;
    else { buf[len++] = (uint8_t)((hi << 4) | h); hi = -1; }
  }
  if (!okHex) { cl->disconnect(); return "{\"error\":\"hex\"}"; }
  if (hi >= 0) { cl->disconnect(); return "{\"error\":\"hexodd\"}"; }

  bool ok = ch->writeValue(buf, len, true);
  out += ",\"ok\":" + String(ok ? "true" : "false") + ",\"size\":" + String(len);
  out += "}";
  cl->disconnect();
  return out;
}

class NfClientCb : public BLEClientCallbacks {
  void onConnect(BLEClient *c) override {}
  void onDisconnect(BLEClient *c) override {
    g_nfOn = false;
  }
};

String BleScanner::gattNotify(const String &addr, const String &svc, const String &chr, bool enable) {
  stopScan();

  if (!enable) {
    if (g_nfClient != nullptr) {
      g_nfClient->disconnect();
      g_nfClient = nullptr;
    }
    g_nfOn = false;
    return "{\"ok\":true,\"on\":false}";
  }

  if (addr.length() != 17) return "{\"error\":\"addr\"}";

  if (g_nfClient == nullptr) {
    g_nfClient = BLEDevice::createClient();
    if (g_nfClient == nullptr) return "{\"error\":\"client\"}";
    g_nfClient->setClientCallbacks(new NfClientCb());
  }
  if (!g_nfClient->isConnected()) {
    if (!g_nfClient->connect(BLEAddress(addr, addrTypeFor(addr.c_str())), 0xFF, 6000)) {
      g_nfClient->disconnect();
      return "{\"error\":\"connect\"}";
    }
  }
  BLERemoteService *sv = g_nfClient->getService(BLEUUID(svc.c_str()));
  if (sv == nullptr) return "{\"error\":\"service\"}";
  BLERemoteCharacteristic *ch = sv->getCharacteristic(BLEUUID(chr.c_str()));
  if (ch == nullptr) return "{\"error\":\"char\"}";

  String key = svc + "|" + chr;
  ch->registerForNotify(
      [key](BLERemoteCharacteristic *c, uint8_t *data, size_t len, bool isNotify) {
        nfStore(key, data, len);
      },
      true);

  strncpy(g_nfAddr, addr.c_str(), sizeof(g_nfAddr) - 1);
  strncpy(g_nfKey, key.c_str(), sizeof(g_nfKey) - 1);
  g_nfOn  = true;
  g_nfTime = 0;
  return "{\"ok\":true,\"on\":true}";
}

String BleScanner::gattStatus() {
  String out = "{\"on\":" + String(g_nfOn ? "true" : "false");
  out += ",\"addr\":\"" + jesc(g_nfAddr) + "\"";
  out += ",\"key\":\"" + jesc(g_nfKey) + "\"";
  if (g_nfHasValue) {
    out += ",\"hex\":\"" + String(g_nfHex) + "\"";
    out += ",\"ascii\":\"" + jesc(g_nfAscii) + "\"";
    out += ",\"time\":" + String(g_nfTime);
  }
  out += "}";
  return out;
}

void BleScanner::disconnectAll() {
  stopScan();
  gattNotify("", "", "", false);
}