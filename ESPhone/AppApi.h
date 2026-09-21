#ifndef ES_APP_API_H
#define ES_APP_API_H

#include <Arduino.h>
#include <WebServer.h>
#include "Protocol.h"
#include "AppRegistry.h"
#include "FileSystem.h"

// Camada "/api" do ESPhone (JSON puro — o ESP32 NAO serve HTML; o app
// Android/nativo renderiza e chama estas rotas por HTTP ou BLE).
// MESMO JSON em WiFi e BLE: o renderer nativo é o unico que desenha.

class AppApi {
public:
  void mount(WebServer &srv, FileSystemClass *fs, AppRegistryClass *reg) {
    fs_  = fs;
    reg_ = reg;
    srv.on("/api/state", HTTP_GET, [=]() { handleStateJson(); });
    srv.on("/api/apps",  HTTP_GET, [=]() { handleAppsJson(); });
    srv.on("/api/app",   HTTP_POST,[=]() { handleAppDispatch(); });
    srv.on("/api/screen",HTTP_POST,[=]() { handleScreenPush(); });
    Serial.println("[Api] rotas JSON montadas: /api/state /api/apps /api/app /api/screen");
  }

  void handleStateJson() {
    String apps  = reg_ ? reg_->appsJson() : String("[]");
    String state = reg_->has("notes") ? reg_->stateJson("notes") : String("{\"app\":\"\"}");
    String body  = Protocol::stateJson(apps, "home", false, state, Protocol::ledJson(1, 255, false));
    srvJson(200, body);
  }

  void handleAppsJson() {
    String list = reg_ ? reg_->appsJson() : String("[]");
    String body = String("{\"t\":\"apps\",\"list\":") + list + "}";
    srvJson(200, body);
  }

  void handleAppDispatch() {
    String appId  = srv_->arg("app");
    String action = srv_->arg("action");
    if (appId.length() == 0) { srvJson(400, Protocol::errJson("app_invalido")); return; }
    if (!reg_) { srvJson(500, Protocol::errJson("sem_registry")); return; }
    String resp = reg_->dispatch(appId, action);
    if (resp.length() == 0) resp = Protocol::errJson("app_desconhecido");
    srvJson(200, resp);
  }

  void handleScreenPush() {
    srvJson(200, Protocol::screenJson(srv_->arg("screen"), srv_->arg("app")));
  }

private:
  void srvJson(int code, const String &body) {
    if (!srv_) return;
    srv_->send(code, "application/json", body);
  }

  WebServer      *srv_ = nullptr;
  FileSystemClass *fs_  = nullptr;
  AppRegistryClass *reg_ = nullptr;
};

extern AppApi api;

#endif
