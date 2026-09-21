#ifndef ES_BLE_H
#define ES_BLE_H

#include <Arduino.h>
#include "AppManager.h"

// Estado global do BLE (lido pela API web)
extern bool g_bleRunning;
extern bool g_bleConnected;

class BleService {
public:
  void begin(AppManager *app);
  void update();   // envia notify quando o estado muda
  void reload();   // reinicia com novo nome do dispositivo
};

// Chamado quando o nome do dispositivo muda (pela web)
void bleNotifyConfigChanged();

extern BleService bleSvc;

#endif