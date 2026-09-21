#include "Protocol.h"

String Protocol::stateJson(const String &appsJson,
                           const String &currentApp,
                           bool inApp,
                           const String &appState,
                           const String &ledJson) {
  String s = "{\"t\":\"state\",\"v\":2";
  s += ",\"apps\":" + appsJson;
  s += ",\"current\":\"" + currentApp + "\"";
  s += ",\"inApp\":" + String(inApp ? "true" : "false");
  s += ",\"app\":" + (appState.length() ? appState : String("{}"));
  s += ",\"led\":" + (ledJson.length() ? ledJson : String("{}"));
  s += "}";
  return s;
}

String Protocol::screenJson(const String &appId, const String &screen) {
  String s = "{\"t\":\"screen\",\"v\":2";
  s += ",\"app\":\"" + appId + "\"";
  s += ",\"screen\":" + (screen.length() ? screen : String("{}"));
  s += "}";
  return s;
}

String Protocol::actionOkJson(const String &appId, const String &result) {
  String s = "{\"t\":\"action_ok\",\"v\":2";
  s += ",\"app\":\"" + appId + "\"";
  s += ",\"result\":" + (result.length() ? result : String("{}"));
  s += "}";
  return s;
}

String Protocol::errJson(const String &code) {
  return String("{\"t\":\"err\",\"v\":2,\"code\":\"") + code + "\"}";
}

String Protocol::ledJson(int mode, int brightness, bool invert) {
  String s = "{\"mode\":" + String(mode);
  s += ",\"bri\":" + String(brightness);
  s += ",\"invert\":" + String(invert ? "true" : "false");
  s += "}";
  return s;
}
