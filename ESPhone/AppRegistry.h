#ifndef ES_APPS_H
#define ES_APPS_H

#include <Arduino.h>
#include <LittleFS.h>
#include "FileSystem.h"

// ------------------------------------------------------------------
// Registro de apps (header-only). Cada app tem um ID unico e vive no
// LittleFS em /apps/<id>/ (+ dados em /data/<id>/). O ESP32 NAO serve
// HTML — o contrato com o app nativo Android é JSON ("o que desenhar").
//
// registerApp(AppManifest) → adiciona ao catálogo; nada mais precisa
// de mudar no firmware para um app novo existir. As rotas são montadas
// dinamicamente pelo WebPortal/BLE a partir de idsJson()/appsJson().
// ------------------------------------------------------------------

struct AppManifest {
  const char *id;     // ex. "notas"
  const char *name;   // ex. "Notas"
  const char *icon;   // id do ícone usado pelo renderer nativo
};

typedef std::function<String(const String &appId)>                AppStateFn;
typedef std::function<String(const String &appId, const String &action)> AppActionFn;

class AppRegistryClass {
public:
  void registerApp(const AppManifest &m,
                   AppStateFn stateFn = nullptr,
                   AppActionFn actionFn = nullptr) {
    if (count_ >= kMax) { Serial.printf("[Reg] limite %d, '%s' ignorado\n", kMax, m.id); return; }
    for (int i = 0; i < count_; i++) {
      if (strcmp(entries_[i].id, m.id) == 0) {   // re-registro: atualiza
        entries_[i].id       = m.id;
        entries_[i].name     = m.name;
        entries_[i].icon     = m.icon;
        entries_[i].stateFn  = stateFn;
        entries_[i].actionFn = actionFn;
        return;
      }
    }
    entries_[count_].id       = m.id;
    entries_[count_].name     = m.name;
    entries_[count_].icon     = m.icon;
    entries_[count_].stateFn  = stateFn;
    entries_[count_].actionFn = actionFn;
    count_++;
    Serial.printf("[Reg] app '%s' (%s) registrado (%d/%d)\n", m.id, m.name, count_, kMax);
  }

  int count() const { return count_; }
  bool has(const char *id) const { return find(id) != nullptr; }

  // JSON que o app nativo consome:
  String idsJson() const {          // ["notas","bluetooth",...]
    String out = "[";
    for (int i = 0; i < count_; i++) { if (i) out += ","; out += String("\"") + entries_[i].id + "\""; }
    return out + "]";
  }
  String appsJson() const {         // [{"id":..,"name":..,"icon":..},...]
    String out = "[";
    for (int i = 0; i < count_; i++) {
      if (i) out += ",";
      out += String("{\"id\":\"") + entries_[i].id + "\"," +
                    "\"name\":\"" + entries_[i].name + "\"," +
                    "\"icon\":\"" + entries_[i].icon + "\"}";
    }
    return out + "]";
  }

  // Estado de tela do app, formato propagado por WiFi HTTP e BLE.
  String stateJson(const String &appId) {
    AppManifestEx *m = find(appId.c_str());
    if (!m) return String();
    if (!m->stateFn) return String("{\"app\":\"") + appId + "\",\"screen\":{}}";
    return m->stateFn(appId);
  }

  // Ação vinda do app nativo (CLICK, SET, SAVE, BACK...) → JSON de resposta.
  String dispatch(const String &appId, const String &action) {
    AppManifestEx *m = find(appId.c_str());
    if (!m) return String("{\"error\":\"app_desconhecido\"}");
    if (!m->actionFn) return String("{\"error\":\"app_sem_acao\"}");
    return m->actionFn(appId, action);
  }

  String appDir(const String &appId) const     { return String("/apps/")  + appId; }
  String appDataDir(const String &appId) const { return String("/data/")  + appId; }
  String configPath(const String &name) const  { return String("/config/") + name; }

private:
  static const int kMax = 16;
  struct AppManifestEx : AppManifest {
    AppStateFn  stateFn;
    AppActionFn actionFn;
  };
  AppManifestEx entries_[kMax];
  int count_ = 0;

  AppManifestEx *find(const char *id) {
    for (int i = 0; i < count_; i++)
      if (strcmp(entries_[i].id, id) == 0) return &entries_[i];
    return nullptr;
  }
  const AppManifestEx *find(const char *id) const {
    return const_cast<AppRegistryClass *>(this)->find(id);
  }
};

extern AppRegistryClass Apps;

#endif
