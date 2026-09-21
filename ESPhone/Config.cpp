#include "Config.h"
#include <Preferences.h>

static Preferences prefs;

ConfigClass Config;

void ConfigClass::loadDefaults() {
  cfg_.deviceName     = "ESPhone";
  cfg_.theme          = "auto";
  cfg_.accent         = 0x21A0DC;
  cfg_.ledBrightness  = 255;
  cfg_.ledInvert      = false;
  cfg_.apEnabled      = true;
  cfg_.apSsid         = "ESPhone-0000";
  cfg_.apPassword     = "esphone123";
  cfg_.staSsid        = "";
  cfg_.staPassword    = "";
  cfg_.scanSpeakPosPin  = 12;  // alto-falante externo: terminal POSITIVO
  cfg_.scanSpeakNegPin  = 13;  // alto-falante externo: terminal NEGATIVO (contrafase)

  uint32_t lo = (uint32_t)(ESP.getEfuseMac() & 0x0000FFFF);
  char suf[16];
  snprintf(suf, sizeof(suf), "%04X", lo);
  cfg_.apSsid = String("ESPhone-") + suf;

  // LED externo radar
  cfg_.extLedOn = false;
  cfg_.extLedInvert = false;
  cfg_.extLedPosPin = 0;
  cfg_.extLedNegPin = 0;
  // Busca intermitente
  cfg_.intermittentScan = false;
  cfg_.intermittentIntervalMs = 30000;
  // Caça-WiFi
  cfg_.wifiHunter = false;
  cfg_.hunterInterval = 30;
}

void ConfigClass::init() {
  loadDefaults();

  // Abre em leitura-escrita: garante que o namespace seja criado
  // na primeira inicialização (abertura somente-leitura falharia).
  if (!prefs.begin("esphone", false)) {
    Serial.println("[Config] NVS indisponível, usando padrões");
    return;
  }
  String dname = prefs.getString("dname", cfg_.deviceName);
  if (dname.length() > 0) cfg_.deviceName = dname;

  cfg_.theme          = prefs.getString("theme", cfg_.theme);
  cfg_.accent         = prefs.getLong("accent", cfg_.accent);
  cfg_.ledBrightness  = prefs.getUChar("ledbri",   cfg_.ledBrightness);
  cfg_.ledInvert      = prefs.getBool("ledinv",    cfg_.ledInvert);
  cfg_.ledScanExt     = prefs.getBool("scanx",     cfg_.ledScanExt);
  cfg_.scanSpeakPosPin  = prefs.getUChar("scanp",    cfg_.scanSpeakPosPin);
  cfg_.scanSpeakNegPin  = prefs.getUChar("scann",    cfg_.scanSpeakNegPin);
  cfg_.apEnabled      = prefs.getBool("apen",      cfg_.apEnabled);
  cfg_.apShareInternet= prefs.getBool("apshr",    cfg_.apShareInternet);

  String s = prefs.getString("apssid", cfg_.apSsid);

  // LED externo radar
  cfg_.extLedOn = prefs.getBool("extlon", cfg_.extLedOn);
  cfg_.extLedInvert = prefs.getBool("extlinv", cfg_.extLedInvert);
  cfg_.extLedPosPin = prefs.getUChar("extlpos", cfg_.extLedPosPin);
  cfg_.extLedNegPin = prefs.getUChar("extlneg", cfg_.extLedNegPin);
  // Busca intermitente
  cfg_.intermittentScan = prefs.getBool("intscan", cfg_.intermittentScan);
  cfg_.intermittentIntervalMs = prefs.getUShort("intint", cfg_.intermittentIntervalMs);
  // Caça-WiFi
  cfg_.wifiHunter = prefs.getBool("wfhunt", cfg_.wifiHunter);
  cfg_.hunterInterval = prefs.getUShort("huntint", cfg_.hunterInterval);

  if (s.length() > 0) cfg_.apSsid = s;
  cfg_.apPassword = prefs.getString("appass", cfg_.apPassword);
  cfg_.staSsid    = prefs.getString("stassid", cfg_.staSsid);
  cfg_.staPassword= prefs.getString("stapass", cfg_.staPassword);
  cfg_.uiPin      = prefs.getString("uipin", "");

  prefs.end();

  if (cfg_.ledBrightness == 0) cfg_.ledBrightness = 255;

  Serial.printf("[Config] nome=%s  AP=%s  STA=%s  tema=%s\n",
                cfg_.deviceName.c_str(),
                cfg_.apSsid.c_str(),
                cfg_.staSsid.length() ? cfg_.staSsid.c_str() : "(off)",
                cfg_.theme.c_str());
}

void ConfigClass::save() {
  prefs.begin("esphone", false);
  prefs.putString("dname",   cfg_.deviceName);
  prefs.putString("theme",   cfg_.theme);
  prefs.putLong("accent",    cfg_.accent);
  prefs.putUChar("ledbri",   cfg_.ledBrightness);
  prefs.putBool("ledinv",    cfg_.ledInvert);
  prefs.putBool("scanx",     cfg_.ledScanExt);
  prefs.putUChar("scanp",    cfg_.scanSpeakPosPin);
  prefs.putUChar("scann",    cfg_.scanSpeakNegPin);
  prefs.putBool("apen",      cfg_.apEnabled);
  prefs.putBool("apshr",    cfg_.apShareInternet);
  prefs.putString("apssid",  cfg_.apSsid);
  prefs.putString("appass",  cfg_.apPassword);
  prefs.putString("stassid", cfg_.staSsid);
  prefs.putString("stapass", cfg_.staPassword);
  prefs.putString("uipin",   cfg_.uiPin);

  // LED externo radar
  prefs.putBool("extlon",   cfg_.extLedOn);
  prefs.putBool("extlinv",  cfg_.extLedInvert);
  prefs.putUChar("extlpos", cfg_.extLedPosPin);
  prefs.putUChar("extlneg", cfg_.extLedNegPin);
  // Busca intermitente
  prefs.putBool("intscan",  cfg_.intermittentScan);
  prefs.putUShort("intint", cfg_.intermittentIntervalMs);
  // Caça-WiFi
  prefs.putBool("wfhunt",  cfg_.wifiHunter);
  prefs.putUShort("huntint", cfg_.hunterInterval);
  prefs.end();
  Serial.println("[Config] salvo");
}

void ConfigClass::saveNotes(const String &notes) {
  prefs.begin("esphone", false);
  prefs.putString("notes", notes.substring(0, NOTE_MAX_LEN));
  prefs.end();
}

String ConfigClass::getNotes() {
  prefs.begin("esphone", true);
  String n = prefs.getString("notes", "");
  prefs.end();
  return n;
}

void ConfigClass::reset() {
  prefs.begin("esphone", false);
  prefs.clear();
  prefs.end();
  loadDefaults();
  save();
  Serial.println("[Config] resetado para padrões");
}