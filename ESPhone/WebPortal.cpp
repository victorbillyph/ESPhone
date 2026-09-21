#include "WebPortal.h"
#include <WiFi.h>
#include <map>
#include "BleScanner.h"
#include "WifiSniffer.h"

// ------------------------------------------------------------------
// Sistema de autenticação PIN (sessão simples em memória)
// ------------------------------------------------------------------
static const uint32_t SESSION_TTL_MS = 30 * 60 * 1000; // 30 min
struct Session {
  uint32_t expiry;
  bool     pinSet;   // true se PIN já foi configurado
};
static std::map<String, Session> sessions;

static String genToken() {
  char buf[33];
  for (int i = 0; i < 32; i++) {
    buf[i] = "0123456789abcdef"[esp_random() % 16];
  }
  buf[32] = 0;
  return String(buf);
}

static void cleanupSessions() {
  uint32_t now = millis();
  for (auto it = sessions.begin(); it != sessions.end(); ) {
    if (now - it->second.expiry > SESSION_TTL_MS) it = sessions.erase(it);
    else ++it;
  }
}

static String getSessionToken(WebServer &server) {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    int p = cookie.indexOf("espsid=");
    if (p >= 0) {
      p += 7;
      int e = cookie.indexOf(';', p);
      if (e < 0) e = cookie.length();
      return cookie.substring(p, e);
    }
  }
  return "";
}

static bool checkAuth(WebServer &, bool &pinConfigured) {
  // PIN/autenticação permanentemente desativados — acesso livre a todos os endpoints.
  pinConfigured = true;
  return true;
}

static void setAuthCookie(WebServer &server, const String &token, bool pinSet) {
  sessions[token] = {millis() + SESSION_TTL_MS, pinSet};
  String cookie = "espsid=" + token + "; Path=/; HttpOnly; Max-Age=1800";
  server.sendHeader("Set-Cookie", cookie);
}

static void clearAuthCookie(WebServer &server) {
  String token = getSessionToken(server);
  if (token.length()) sessions.erase(token);
  server.sendHeader("Set-Cookie", "espsid=; Path=/; HttpOnly; Max-Age=0");
}

// ------------------------------------------------------------------
// Mini utilitários de JSON (sem dependência externa)
// ------------------------------------------------------------------
static String jesc(const String &s) {
  String r = s;
  r.replace("\\", "\\\\");
  r.replace("\"", "\\\"");
  r.replace("\n", "\\n");
  r.replace("\r", "\\r");
  return r;
}

static String jsStr(const String &body, const char *key) {
  String k = String("\"") + key + "\":\"";
  int p = body.indexOf(k);
  if (p < 0) return String();
  p += k.length();
  int e = body.indexOf('"', p);
  if (e < 0) return String();
  return body.substring(p, e);
}

static long jsInt(const String &body, const char *key, long dflt) {
  String k = String("\"") + key + "\":";
  int p = body.indexOf(k);
  if (p < 0) return dflt;
  p += k.length();
  int e = p;
  while (e < (int)body.length() && (isDigit(body[e]) || body[e] == '-')) e++;
  if (e == p) return dflt;
  return body.substring(p, e).toInt();
}

static bool jsBool(const String &body, const char *key, bool dflt) {
  String k = String("\"") + key + "\":";
  int p = body.indexOf(k);
  if (p < 0) return dflt;
  String rest = body.substring(p + k.length());
  if (rest.startsWith("true")) return true;
  if (rest.startsWith("false")) return false;
  return dflt;
}

static String accentHex(long acc) {
  char b[8];
  snprintf(b, sizeof(b), "#%02X%02X%02X",
           (int)((acc >> 16) & 0xFF), (int)((acc >> 8) & 0xFF), (int)(acc & 0xFF));
  return String(b);
}

// ------------------------------------------------------------------
// WiFi
// ------------------------------------------------------------------
bool WebPortal::startApIfEnabled() {
  DeviceConfig &c = Config.get();
  if (!c.apEnabled) return false;
  bool ok = WiFi.softAP(c.apSsid.c_str(),
                        c.apPassword.length() ? c.apPassword.c_str() : NULL,
                        1, 0, 1);
  if (ok) {
    Serial.printf("[WiFi] AP %s  ->  http://%s\n",
                  c.apSsid.c_str(), WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[WiFi] falha ao iniciar o AP");
  }
  return ok;
}

void WebPortal::setupWifi() {
  DeviceConfig &c = Config.get();

  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);

  startApIfEnabled();

  if (c.staSsid.length() > 0) {
    // Varredura rápida: LED dá o feedback (2x = achou redes, 1x = achou a
    // rede conhecida). Só no boot para não atrapalhar a conexão.
    int n = WiFi.scanNetworks();
    if (app_ && n > 0) {
      app_->notify(2, 120, 120);            // achou redes WiFi
      bool known = false;
      for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == c.staSsid) { known = true; break; }
      }
      if (known) app_->notify(1, 150, 150); // achou a rede conhecida
    }
    WiFi.scanDelete();

    Serial.printf("[WiFi] conectando na rede %s...\n", c.staSsid.c_str());
    WiFi.begin(c.staSsid.c_str(), c.staPassword.c_str());
    staTrying_ = true;
    staStart_  = millis();
  } else {
    if (app_) app_->ledConnecting(false);
  }
}

void WebPortal::reconfigSta(const String &ssid, const String &pass) {
  DeviceConfig &c = Config.get();
  c.staSsid = ssid;
  c.staPassword = pass;
  Config.save();

  WiFi.disconnect(false, true);
  WiFi.enableSTA(true);

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), pass.c_str());
    staTrying_ = true;
    staStart_  = millis();
    Serial.printf("[WiFi] reconectando STA %s\n", ssid.c_str());
  } else {
    staTrying_ = false;
    WiFi.enableSTA(false);
    Serial.println("[WiFi] STA desligado");
  }
}

// ------------------------------------------------------------------
// Servidor
// ------------------------------------------------------------------
void WebPortal::begin(AppManager *app) {
  app_ = app;

  setupWifi();

  server.on("/", HTTP_GET, [this] { handleRoot(); });
  server.on("/api/state", HTTP_GET, [this] { handleState(); });
  server.on("/api/info", HTTP_GET, [this] { handleInfo(); });
  server.on("/api/config", HTTP_GET, [this] { handleConfigGet(); });
  server.on("/api/config", HTTP_POST, [this] { handleConfigPost(); });
  server.on("/api/wifi", HTTP_POST, [this] { handleWifiPost(); });
  server.on("/api/nav", HTTP_POST, [this] { handleNav(); });
  server.on("/api/notes", HTTP_GET, [this] { handleNotesGet(); });
  server.on("/api/notes", HTTP_POST, [this] { handleNotesPost(); });
  server.on("/api/ble/scan", HTTP_GET, [this] { handleBleScan(); });
  server.on("/api/ble/scan", HTTP_POST, [this] { handleBleScanPost(); });
  server.on("/api/ble/saved", HTTP_GET, [this] { handleBleSaved(); });
  server.on("/api/ble/save", HTTP_POST, [this] { handleBleSave(); });
  server.on("/api/ble/forget", HTTP_POST, [this] { handleBleForget(); });
  server.on("/api/ble/gatt", HTTP_POST, [this] { handleBleGatt(); });
  server.on("/api/ble/gatt/read", HTTP_POST, [this] { handleBleGattRead(); });
  server.on("/api/ble/gatt/write", HTTP_POST, [this] { handleBleGattWrite(); });
  server.on("/api/ble/gatt/notify", HTTP_POST, [this] { handleBleGattNotify(); });
  server.on("/api/ble/gatt/status", HTTP_GET, [this] { handleBleGattStatus(); });
  server.on("/api/wifi/monitor", HTTP_GET, [this] { handleWifiMonitor(); });
  server.on("/api/wifi/monitor", HTTP_POST, [this] { handleWifiMonitorPost(); });
  server.on("/api/wifi/hunter", HTTP_POST, [this] { handleWifiHunter(); });
  server.on("/api/reboot", HTTP_POST, [this] { handleReboot(); });
  server.on("/api/reset", HTTP_POST, [this] { handleReset(); });
  server.on("/api/self-test", HTTP_POST, [this] { handleSelfTest(); });
  server.on("/api/motor/probe", HTTP_GET, [this] { handleMotorProbe(); });
  server.on("/api/gpio", HTTP_GET, [this] { handleGpioGet(); });
  server.on("/api/gpio", HTTP_POST, [this] { handleGpioPost(); });
  server.on("/api/pin/setup", HTTP_POST, [this] { handlePinSetup(); });
  server.on("/api/pin/verify", HTTP_POST, [this] { handlePinVerify(); });
  server.on("/api/pin/status", HTTP_GET, [this] { handlePinStatus(); });
  server.on("/api/logout", HTTP_POST, [this] { handleLogout(); });

  server.begin();
  began_ = true;
  Serial.println("[Web] servidor HTTP na porta 80");
}

void WebPortal::update() {
  if (began_) server.handleClient();

  // Máquina de estados da conexão STA (não bloqueante)
  if (staTrying_) {
    if (WiFi.status() == WL_CONNECTED) {
      staTrying_ = false;
      Serial.printf("[WiFi] STA conectado -> IP %s\n",
                    WiFi.localIP().toString().c_str());
      if (Config.get().apShareInternet && Config.get().apEnabled) {
        Serial.println("[NAT] Toggle ativo — NAT requer core com CONFIG_LWIP_IPV4_NAPT=1 (rebuild do core ESP32 Arduino). Toggle persistido.");
      } else if (Config.get().apShareInternet) {
        Serial.println("[NAT] Não compartilhado: STA inativo ou AP desligado");
      }
      if (app_) { app_->ledReady(); app_->notify(3, 120, 120); }  // conectou: 3x
    } else if (millis() - staStart_ > 15000) {
      staTrying_ = false;
      Serial.println("[WiFi] STA falhou (timeout), mantendo o AP");
      WiFi.disconnect(false, true);
      WiFi.enableSTA(false);
      if (app_) app_->ledConnecting(false);
    } else {
      if (app_) app_->ledConnecting(true);
    }
  }

  if (rebootPending_) {
    Serial.println("[Web] reiniciando...");
    delay(200);
    ESP.restart();
  }
}

// ------------------------------------------------------------------
// Console serial (configuração de WiFi, AP e nome na NVS)
// ------------------------------------------------------------------
String WebPortal::takeToken(const String &s, int &i) {
  while (i < (int)s.length() && s[i] == ' ') i++;
  String t;
  if (i < (int)s.length() && s[i] == '"') {
    i++;
    while (i < (int)s.length() && s[i] != '"') t += s[i++];
    if (i < (int)s.length()) i++;
  } else {
    while (i < (int)s.length() && s[i] != ' ') t += s[i++];
  }
  return t;
}

void WebPortal::printSerialHelp() {
  Serial.println();
  Serial.println("=== ESPhone - comandos seriais ===");
  Serial.println("  wifi <ssid> <senha>   conecta na sua rede (salva na NVS)");
  Serial.println("  wifi off              desconecta e limpa a rede salva");
  Serial.println("  ap <ssid> <senha>     define o Access Point");
  Serial.println("  ap on | ap off        liga/desliga o Access Point");
  Serial.println("  name <nome>           nome do dispositivo");
  Serial.println("  status                mostra a configuracao atual");
  Serial.println("  reboot                reinicia");
  Serial.println("  reset                 restaura de fabrica");
  Serial.println("  help                  mostra esta ajuda");
  Serial.println("Dica: use aspas p/ nomes com espaco, ex.: wifi \"Minha Rede\" senha123");
  Serial.println("==================================");
}

void WebPortal::runSerialCommand(String line) {
  int sp = line.indexOf(' ');
  String cmd = sp < 0 ? line : line.substring(0, sp);
  String args = sp < 0 ? String() : line.substring(sp + 1);
  cmd.toLowerCase();
  args.trim();

  DeviceConfig &c = Config.get();

  if (cmd == "help" || cmd == "?") {
    printSerialHelp();
    return;
  }

  if (cmd == "status" || cmd == "info") {
    wl_status_t s = WiFi.status();
    bool staUp = (s == WL_CONNECTED);
    Serial.printf("[Status] nome=%s  AP=%s (%s)  STA=%s  IP=%s  heap=%u\n",
                  c.deviceName.c_str(), c.apSsid.c_str(),
                  c.apEnabled ? "on" : "off",
                  c.staSsid.length() ? c.staSsid.c_str() : "(off)",
                  staUp ? WiFi.localIP().toString().c_str()
                        : WiFi.softAPIP().toString().c_str(),
                  (unsigned)ESP.getFreeHeap());
    return;
  }

  if (cmd == "wifi") {
    int i = 0;
    String ssid = takeToken(args, i);
    String pass = takeToken(args, i);
    if (ssid.length() == 0) {
      Serial.println("[WiFi] uso: wifi <ssid> <senha>  |  wifi off");
      return;
    }
    if (ssid.equalsIgnoreCase("off") || ssid.equalsIgnoreCase("none")) {
      reconfigSta("", "");
      Serial.println("[WiFi] rede STA removida (modo AP)");
      return;
    }
    Serial.printf("[WiFi] salvando e conectando em \"%s\"...\n", ssid.c_str());
    reconfigSta(ssid, pass);
    return;
  }

  if (cmd == "ap") {
    int i = 0;
    String a1 = takeToken(args, i);
    String a2 = takeToken(args, i);
    if (a1.equalsIgnoreCase("on")) {
      c.apEnabled = true; Config.save();
      startApIfEnabled();
      Serial.println("[AP] ligado");
      return;
    }
    if (a1.equalsIgnoreCase("off")) {
      c.apEnabled = false; Config.save();
      WiFi.softAPdisconnect(true);
      Serial.println("[AP] desligado");
      return;
    }
    if (a1.length() == 0) {
      Serial.println("[AP] uso: ap <ssid> <senha> | ap on | ap off");
      return;
    }
    c.apEnabled = true;
    c.apSsid = a1;
    c.apPassword = a2;
    Config.save();
    WiFi.softAPdisconnect(true);
    startApIfEnabled();
    Serial.printf("[AP] definido: %s\n", a1.c_str());
    return;
  }

  if (cmd == "name") {
    if (args.length() == 0) { Serial.println("[Nome] uso: name <nome>"); return; }
    String old = c.deviceName;
    c.deviceName = args;
    Config.save();
    if (c.deviceName != old) bleNotifyConfigChanged();
    Serial.printf("[Nome] %s\n", c.deviceName.c_str());
    return;
  }

  if (cmd == "reboot") {
    Serial.println("[Sistema] reiniciando...");
    delay(150);
    ESP.restart();
    return;
  }

  if (cmd == "pin off") {
    Config.get().uiPin = "";
    Config.save();
    sessions.clear();
    Serial.println("[PIN] removido. Crie um novo PIN pela interface web.");
    return;
  }
  if (cmd == "reset") {
    Serial.println("[Sistema] restaurando de fabrica...");
    Config.reset();
    delay(150);
    ESP.restart();
    return;
  }

  Serial.printf("[Serial] comando desconhecido: \"%s\" (digite help)\n", cmd.c_str());
}

void WebPortal::handleSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      serialBuf_.trim();
      if (serialBuf_.length()) runSerialCommand(serialBuf_);
      serialBuf_ = "";
    } else if (ch == 8 || ch == 127) {
      if (serialBuf_.length()) serialBuf_.remove(serialBuf_.length() - 1);
    } else if (serialBuf_.length() < 200) {
      serialBuf_ += ch;
    }
  }
}

void WebPortal::handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  sendJson(200,
      "{\"t\":\"index\",\"ok\":true,"
      "\"endpoints\":["
      "\"/api/state\",\"/api/apps\",\"/api/config\",\"/api/ble/scan\","
      "\"/api/ble/saved\",\"/api/wifi/monitor\"]}");
}

// ------------------------------------------------------------------
// API
// ------------------------------------------------------------------
void WebPortal::handleState() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  DeviceConfig &c = Config.get();

  String apps = "[";
  for (int i = 0; i < APP_COUNT; i++) {
    apps += String("\"") + jesc(APP_NAMES[i]) + "\"";
    if (i < APP_COUNT - 1) apps += ",";
  }
  apps += "]";

  wl_status_t s = WiFi.status();
  bool staUp = (s == WL_CONNECTED);
  String mode = staUp ? "STA" : (c.apEnabled ? "AP" : "off");
  String ip = staUp ? WiFi.localIP().toString()
                    : (c.apEnabled ? WiFi.softAPIP().toString() : "-");
  String ssid = staUp ? c.staSsid : (c.apEnabled ? c.apSsid : "-");
  long rssi = staUp ? (long)WiFi.RSSI() : 0;

  String json = String("{")
      + "\"deviceName\":\"" + jesc(c.deviceName) + "\","
      + "\"version\":\"" ESPhone_VERSION "\","
      + "\"appIndex\":" + String(app_->index()) + ","
      + "\"appCount\":" + String(APP_COUNT) + ","
      + "\"inApp\":" + String(app_->inApp() ? "true" : "false") + ","
      + "\"apps\":" + apps + ","
      + "\"led\":{\"pattern\":\"" + jesc(app_->ledPatternName())
      + "\",\"bright\":" + String((int)c.ledBrightness) + "},"
       + "\"ble\":" + String(g_bleRunning ? "true" : "false")
       + ",\"bleConnected\":" + String(g_bleConnected ? "true" : "false") + ","
       + "\"wifi\":{\"mode\":\"" + mode + "\",\"ssid\":\"" + jesc(ssid)
      + "\",\"sta\":" + String(staUp ? "true" : "false")
      + ",\"enabled\":" + String((c.apEnabled || staUp) ? "true" : "false")
      + ",\"rssi\":" + String(rssi)
      + ",\"clients\":" + String(WiFi.softAPgetStationNum())
      + ",\"ip\":\"" + ip + "\"}"
      "}";

  sendJson(200, json);
}

void WebPortal::handleInfo() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  DeviceConfig &c = Config.get();
  wl_status_t s = WiFi.status();
  bool staUp = (s == WL_CONNECTED);

  String json = String("{")
      + "\"deviceName\":\"" + jesc(c.deviceName) + "\","
      + "\"version\":\"" ESPhone_VERSION "\","
      + "\"chip\":\"" + String(ESP.getChipModel()) + "\","
      + "\"revision\":" + String(ESP.getChipRevision()) + ","
      + "\"cores\":" + String(ESP.getChipCores()) + ","
      + "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ","
      + "\"flashSize\":" + String(ESP.getFlashChipSize()) + ","
      + "\"psram\":" + String(ESP.getPsramSize() > 0 ? "true" : "false") + ","
      + "\"psramSize\":" + String(ESP.getPsramSize()) + ","
      + "\"heapFree\":" + String(ESP.getFreeHeap()) + ","
      + "\"heapMax\":" + String(ESP.getMaxAllocHeap()) + ","
      + "\"temp\":" + String(temperatureRead(), 1) + ","
      + "\"up\":" + String(millis() / 1000) + ","
      + "\"macSta\":\"" + WiFi.macAddress() + "\","
      + "\"macAp\":\"" + WiFi.softAPmacAddress() + "\","
      + "\"wifiMode\":\"" + String(staUp ? "STA" : (c.apEnabled ? "AP" : "off")) + "\","
      + "\"staSsid\":\"" + jesc(c.staSsid) + "\","
      + "\"rssi\":" + String(staUp ? WiFi.RSSI() : 0) + ","
      + "\"apClients\":" + String(WiFi.softAPgetStationNum()) + ","
      + "\"ip\":\"" + String(staUp ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\","
      + "\"ble\":" + String(g_bleRunning ? "true" : "false")
      + "}";

  sendJson(200, json);
}

void WebPortal::handleConfigGet() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  DeviceConfig &c = Config.get();
  String json = String("{")
      + "\"deviceName\":\"" + jesc(c.deviceName) + "\","
      + "\"theme\":\"" + jesc(c.theme) + "\","
      + "\"accent\":" + String(c.accent) + ","
      + "\"accentHex\":\"" + accentHex(c.accent) + "\","
      + "\"ledBrightness\":" + String((int)c.ledBrightness) + ","
      + "\"ledInvert\":" + String(c.ledInvert ? "true" : "false") + ","
      + "\"ledScanExt\":" + String(c.ledScanExt ? "true" : "false") + ","
+ "\"scanSpeakPosPin\":" + String((int)c.scanSpeakPosPin) + ","
       + "\"scanSpeakNegPin\":" + String((int)c.scanSpeakNegPin) + ","
      + "\"apEnabled\":" + String(c.apEnabled ? "true" : "false") + ","
      + "\"apShareInternet\":" + String(c.apShareInternet ? "true" : "false") + ","
      + "\"apSsid\":\"" + jesc(c.apSsid) + "\","
      + "\"apPassword\":\"" + jesc(c.apPassword) + "\","
      + "\"staSsid\":\"" + jesc(c.staSsid) + "\","
      + "\"staPassword\":\"" + jesc(c.staPassword) + "\""
      "}";
  sendJson(200, json);
}

void WebPortal::handleConfigPost() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  if (body.length() == 0) {
    sendJson(400, "{\"ok\":false}");
    return;
  }

  DeviceConfig &c = Config.get();
  String oldName = c.deviceName;
  bool oldApEn   = c.apEnabled;
  String oldApSsid = c.apSsid;
  String oldApPass = c.apPassword;

  bool touched = false;
  String v;

  if ((v = jsStr(body, "deviceName")).length()) { c.deviceName = v; touched = true; }
  if ((v = jsStr(body, "theme")).length())       { c.theme = v; touched = true; }
  long acc = jsInt(body, "accent", -1);
  if (acc >= 0)                                 { c.accent = acc; touched = true; }
  long bri = jsInt(body, "ledBrightness", -1);
  if (bri >= 1 && bri <= 255)                   { c.ledBrightness = (uint8_t)bri; touched = true; }
  if (body.indexOf("\"ledInvert\"") >= 0)       { c.ledInvert = jsBool(body, "ledInvert", c.ledInvert); touched = true; }
  if (body.indexOf("\"ledScanExt\"") >= 0)      { c.ledScanExt = jsBool(body, "ledScanExt", c.ledScanExt); touched = true; }
  long sp = jsInt(body, "scanSpeakPosPin", -1), sn = jsInt(body, "scanSpeakNegPin", -1);
  if (sp >= 0 && sp <= 255)              { c.scanSpeakPosPin = (uint8_t)sp; touched = true; }
  if (sn >= 0 && sn <= 255)              { c.scanSpeakNegPin = (uint8_t)sn; touched = true; }
  if (body.indexOf("\"apEnabled\"") >= 0)       { c.apEnabled = jsBool(body, "apEnabled", c.apEnabled); touched = true; }
  if (body.indexOf("\"apShareInternet\"") >= 0) { c.apShareInternet = jsBool(body, "apShareInternet", c.apShareInternet); touched = true; }
  if (body.indexOf("\"wifiHunter\"") >= 0) { c.wifiHunter = jsBool(body, "wifiHunter", c.wifiHunter); touched = true; }
  long hi = jsInt(body, "hunterInterval", -1);
  if (hi >= 5 && hi <= 3600) { c.hunterInterval = (uint16_t)hi; touched = true; }
  if ((v = jsStr(body, "apSsid")).length())     { c.apSsid = v; touched = true; }
  if ((v = jsStr(body, "apPassword")).length()) { c.apPassword = v; touched = true; }

  if (!touched) {
    sendJson(400, "{\"ok\":false}");
    return;
  }

  Config.save();
  if (app_) app_->refreshFromConfig();

  // Re-aplica o Access Point somente se os valores mudaram
  bool apChanged = (c.apEnabled != oldApEn) || (c.apSsid != oldApSsid) ||
                   (c.apPassword != oldApPass);
  if (apChanged) {
    WiFi.softAPdisconnect(true);
    startApIfEnabled();
  }

  // Se o nome mudou, reinicia o BLE para anunciar o novo nome
  if (c.deviceName != oldName) {
    bleNotifyConfigChanged();
  }

  sendOk();
}

void WebPortal::handleWifiPost() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  String mode = jsStr(body, "mode");
  String ssid = jsStr(body, "ssid");
  String pass = jsStr(body, "password");

  DeviceConfig &c = Config.get();

  if (mode == "ap") {
    c.apEnabled = true;
    if (ssid.length()) c.apSsid = ssid;
    if (pass.length()) c.apPassword = pass;
    Config.save();
    WiFi.softAPdisconnect(true);
    startApIfEnabled();
    Serial.println("[Web] AP reconfigurado");
    sendOk();
    return;
  }

  if (mode == "sta") {
    reconfigSta(ssid, pass);
    sendOk();
    return;
  }

  sendJson(400, "{\"ok\":false,\"error\":\"mode\"}");
}

void WebPortal::handleNav() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  if (!app_) { sendJson(500, "{\"ok\":false}"); return; }
  String body = server.arg("plain");
  body.trim();
  if (body.length() == 0) { sendJson(400, "{\"ok\":false}"); return; }

  String act = jsStr(body, "action");
  if (act == "goto") {
    app_->dispatch(NAV_GOTO, (int)jsInt(body, "index", 0));
    sendJson(200, "{\"ok\":true,\"appIndex\":" + String(app_->index()) + "}");
    return;
  }

  int a = -1;
  if (act == "prev")   a = NAV_PREV;
  if (act == "next")   a = NAV_NEXT;
  if (act == "select") a = NAV_SELECT;
  if (act == "back")   a = NAV_BACK;
  if (act == "home")   a = NAV_HOME;

  if (a >= 0) {
    app_->dispatch(a);
    sendJson(200, "{\"ok\":true,\"appIndex\":" + String(app_->index())
                  + ",\"inApp\":" + String(app_->inApp() ? "true" : "false") + "}");
  } else {
    sendJson(400, "{\"ok\":false,\"error\":\"action\"}");
  }
}

void WebPortal::handleNotesGet() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String json = String("{\"notes\":\"") + jesc(Config.getNotes()) + "\"}";
  sendJson(200, json);
}

void WebPortal::handleNotesPost() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  if (body.length() == 0) { sendJson(400, "{\"ok\":false}"); return; }
  String notes = jsStr(body, "notes");
  Config.saveNotes(notes);
  sendOk();
}

void WebPortal::handleBleScan() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  sendJson(200, bleScanner.buildScanJson());
}

void WebPortal::handleBleScanPost() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  bool active = jsBool(body, "active", true);
  if (active) {
    sendJson(200, String("{\"ok\":") + (bleScanner.startScan() ? "true" : "false") + "}");
  } else {
    bleScanner.stopScan();
    sendOk();
  }
}

void WebPortal::handleBleSaved() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  sendJson(200, bleScanner.buildSavedJson());
}

void WebPortal::handleBleSave() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  String addr = jsStr(body, "addr");
  String name = jsStr(body, "name");
  if (addr.length() != 17) { sendJson(400, "{\"ok\":false,\"error\":\"addr\"}"); return; }
  bleScanner.saveDevice(addr, name);
  sendOk();
}

void WebPortal::handleBleForget() {
  bool pinConfigured;
  if (!checkAuth(server, pinConfigured)) { sendJson(401, "{\"error\":\"unauthorized\"}"); return; }
  String body = server.arg("plain");
  String addr = jsStr(body, "addr");
  if (addr.length() != 17) { sendJson(400, "{\"ok\":false,\"error\":\"addr\"}"); return; }
  bleScanner.forgetDevice(addr);
  sendOk();
}

void WebPortal::handleBleGatt() {
  String body = server.arg("plain");
  String addr = jsStr(body, "addr");
  sendJson(200, bleScanner.gattDiscover(addr));
}

void WebPortal::handleBleGattRead() {
  String body = server.arg("plain");
  sendJson(200, bleScanner.gattRead(jsStr(body, "addr"), jsStr(body, "svc"), jsStr(body, "chr")));
}

void WebPortal::handleBleGattWrite() {
  String body = server.arg("plain");
  sendJson(200, bleScanner.gattWrite(jsStr(body, "addr"), jsStr(body, "svc"),
                                      jsStr(body, "chr"), jsStr(body, "hex")));
}

void WebPortal::handleBleGattNotify() {
  String body = server.arg("plain");
  bool enable = jsBool(body, "enable", true);
  sendJson(200, bleScanner.gattNotify(jsStr(body, "addr"), jsStr(body, "svc"),
                                      jsStr(body, "chr"), enable));
}

void WebPortal::handleBleGattStatus() {
  sendJson(200, bleScanner.gattStatus());
}

void WebPortal::handleWifiMonitor() {
  sendJson(200, wifiSniffer.buildJson());
}

void WebPortal::handleWifiMonitorPost() {
  String body = server.arg("plain");
  if (body.indexOf("\"on\"") >= 0) {
    bool on = jsBool(body, "on", false);
    wifiSniffer.setEnabled(on);
    if (!on) wifiSniffer.setHop(false);
  }
  if (body.indexOf("\"hop\"") >= 0) {
    wifiSniffer.setHop(jsBool(body, "hop", false));
  }
  sendOk();
}

void WebPortal::handleWifiHunter() {
  String body = server.arg("plain");
  String action = jsStr(body, "action");
  DeviceConfig &c = Config.get();

  if (action == "scan") {
    // Trigger immediate scan
    if (app_) app_->triggerWifiHunterScan();
    sendJson(200, "{\"ok\":true,\"action\":\"scan\"}");
  } else if (action == "toggle") {
    c.wifiHunter = !c.wifiHunter;
    Config.save();
    if (app_) app_->refreshFromConfig();
    sendJson(200, "{\"ok\":true,\"wifiHunter\":" + String(c.wifiHunter ? "true" : "false") + "}");
  } else {
    sendJson(400, "{\"ok\":false,\"error\":\"invalid action\"}");
  }
}

void WebPortal::handleReboot() {
  sendOk();
  rebootPending_ = true;
}

void WebPortal::handleReset() {
  Config.reset();
  sendOk();
  rebootPending_ = true;
}

void WebPortal::handleSelfTest() {
  if (!app_) { sendJson(500, "{\"ok\":false}"); return; }
  String result = app_->selfTest();
  sendJson(200, "{\"ok\":true,\"result\":\"" + jesc(result) + "\"}");
}
void WebPortal::handleMotorProbe() {
  if (!app_) { sendJson(500, "{\"ok\":false}"); return; }
  DeviceConfig &c = Config.get();
  uint8_t pos = c.scanSpeakPosPin, neg = c.scanSpeakNegPin;
  if (!pos || !neg || pos==neg || pos>39 || neg>39) {
    sendJson(400, "{\"ok\":false,\"error\":\"confira os pinos do motor\"}");
    return;
  }
  int last=-1, tr=0, peak=0;
  uint32_t t0=millis();
  while(millis()-t0 < 1500){
    int a=analogRead(pos), b=analogRead(neg);
    int cur=a>b?1:0;
    if(last>=0 && cur!=last) tr++;
    if(a>peak) peak=a;
    if(b>peak) peak=b;
    last=cur;
    delay(2);
  }
  bool detect = (tr>=4) && (peak>800);
  sendJson(200, "{\"ok\":true,\"detect\":"+String(detect?"true":"false")+
     ",\"transitions\":"+String(tr)+",\"peak\":"+String(peak)+"}");
}
void WebPortal::handleGpioGet() {
  if (!app_) { sendJson(500, "{\"ok\":false}"); return; }
  String json = app_->gpioGetAll();
  sendJson(200, json);
}

void WebPortal::handleGpioPost() {
  if (!app_) { sendJson(500, "{\"ok\":false}"); return; }
  String body = server.arg("plain");
  if (body.length() == 0) { sendJson(400, "{\"ok\":false,\"error\":\"empty\"}"); return; }
  
  long pin = jsInt(body, "pin", -1);
  bool on = jsBool(body, "on", false);
  
  if (pin < 0 || pin > 39) {
    sendJson(400, "{\"ok\":false,\"error\":\"pin\"}");
    return;
  }
  
  bool ok = app_->gpioSet((uint8_t)pin, on);
  if (ok) {
    sendJson(200, "{\"ok\":true,\"pin\":" + String(pin) + ",\"on\":" + String(on ? "true" : "false") + "}");
  } else {
    sendJson(403, "{\"ok\":false,\"error\":\"reserved\",\"pin\":" + String(pin) + "}");
  }
}
void WebPortal::handlePinStatus() {
  String set = Config.get().uiPin.length() ? "true" : "false";
  sendJson(200, "{\"ok\":true,\"set\":" + set + "}");
}

void WebPortal::handlePinSetup() {
  bool pinConfigured;
  bool authed = checkAuth(server, pinConfigured);
  String body = server.arg("plain");
  String pin = jsStr(body, "pin");
  if (pin.length() != 4 || pin == "0000") {
    sendJson(400, "{\"ok\":false,\"error\":\"pin-invalid\"}");
    return;
  }
  bool already = Config.get().uiPin.length() > 0;
  if (already && !authed) {
    sendJson(403, "{\"ok\":false,\"error\":\"pin-exists\"}");
    return;
  }
  Config.get().uiPin = pin;
  Config.save();
  for (auto &kv : sessions) kv.second.pinSet = true;
  // Cria sessão+token já no setup, para não voltar ao login após o reload
  setAuthCookie(server, genToken(), true);
  sendJson(200, "{\"ok\":true}");
}

void WebPortal::handlePinVerify() {
  String body = server.arg("plain");
  String pin = jsStr(body, "pin");
  if (pin.length() != 4) { sendJson(400, "{\"ok\":false,\"error\":\"pin-invalid\"}"); return; }
  if (Config.get().uiPin.length() == 0) { sendJson(403, "{\"ok\":false,\"error\":\"pin-not-set\"}"); return; }
  if (pin != Config.get().uiPin) { sendJson(403, "{\"ok\":false,\"error\":\"pin-wrong\"}"); return; }
  setAuthCookie(server, genToken(), true);
  sendJson(200, "{\"ok\":true}");
}

void WebPortal::handleLogout() {
  clearAuthCookie(server);
  sendJson(200, "{\"ok\":true}");
}
