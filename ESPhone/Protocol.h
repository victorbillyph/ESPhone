#ifndef ES_PROTOCOL_H
#define ES_PROTOCOL_H

#include <Arduino.h>

// Contrato JSON entre o ESP32 e o app nativo Android (WiFi HTTP e BLE
// falam EXATAMENTE o mesmo JSON — o app renderiza, o ESP nunca manda
// HTML). Cada pacote tem "t" (tipo): state | screen | err | action_ok.

struct Protocol {
  static String stateJson(const String &appsJson,
                          const String &currentApp,
                          bool inApp,
                          const String &appState,
                          const String &ledJson);
  static String screenJson(const String &appId, const String &screen);
  static String actionOkJson(const String &appId, const String &result);
  static String errJson(const String &code);   // {"t":"err","code":..}
  static String ledJson(int mode, int brightness, bool invert);
};

#endif
