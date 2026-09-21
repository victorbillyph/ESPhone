#ifndef ES_WEBPORTAL_H
#define ES_WEBPORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include "AppManager.h"
#include "BleService.h"

class WebPortal {
public:
  void begin(AppManager *app);
  void update();
  void handleSerial();
  WebServer &serverRef() { return server; }

private:
  void handleRoot();
  void handleState();
  void handleInfo();
  void handleConfigGet();
  void handleConfigPost();
  void handleWifiPost();
  void handleNav();
  void handleNotesGet();
  void handleNotesPost();
  void handleBleScan();
  void handleBleScanPost();
  void handleBleSaved();
  void handleBleSave();
  void handleBleForget();
  void handleBleGatt();
  void handleBleGattRead();
  void handleBleGattWrite();
  void handleBleGattNotify();
  void handleBleGattStatus();
  void handleWifiMonitor();
  void handleWifiMonitorPost();
  void handleWifiHunter();
  void handleReboot();
  void handleReset();
  void handleSelfTest();
  void handleMotorProbe();
  void handleGpioGet();
  void handleGpioPost();
  void handlePinSetup();
  void handlePinVerify();
  void handlePinStatus();
  void handleLogout();

  void setupWifi();
  void reconfigSta(const String &ssid, const String &pass);
  bool startApIfEnabled();

  void runSerialCommand(String line);
  void printSerialHelp();
  static String takeToken(const String &s, int &i);

  void sendJson(int code, const String &body) { server.send(code, "application/json", body); }
  void sendOk() { sendJson(200, "{\"ok\":true}"); }

  AppManager *app_ = nullptr;
  WebServer server{80};
  bool      began_      = false;
  bool      staTrying_  = false;
  uint32_t  staStart_   = 0;
  bool      rebootPending_ = false;
  String    serialBuf_;
};

#endif