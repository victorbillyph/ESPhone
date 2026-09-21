#include "AppManager.h"
#include "BleScanner.h"
#include <WiFi.h>
extern BleScanner bleScanner;

// ============================================================
// Hardware: ajuste para a sua placa
//  - LED embutido: a maioria dos ESP32 DevKit/NodeMCU usa GPIO2
//  - Botão BOOT  : GPIO0 (ativo em nível BAIXO)
// Se a sua placa não tem LED no GPIO2, altere LED_PIN abaixo.
// ============================================================
#define LED_PIN  2
#define BTN_PIN  0
#define BTN_LONG_MS     600ul
#define BTN_DEBOUNCE_MS  30ul

// Sem toque no botão físico por este tempo -> LED apaga (suspensão)
#define SUSPEND_MS     15000ul

AppManager appMgr;

const char *const APP_NAMES[APP_COUNT] = {
  "Configurações",
  "Bluetooth",
  "WiFi Monitor",
  "Sobre",
  "Sistema",
  "Notas",
  "Modulos",
  "GPIO"
};
void AppManager::begin() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  refreshFromConfig();

  lastPhys_ = millis();

  // Boot: 3 piscadas rápidas depois LED aceso (pronto)
  blink(3, LED_SOLID_ON);

  Serial.println("[AppManager] iniciado");
}

void AppManager::refreshFromConfig() {
  DeviceConfig &c = Config.get();
  brightness = c.ledBrightness;
  invert     = c.ledInvert;
  // LED externo radar (novos pinos)
  if (c.extLedOn) {
    setScanLedPins(c.extLedPosPin, c.extLedNegPin);
    // aplica inversão se necessário (já tratado no writeScanLed)
  } else {
    // desliga LED externo se estava ligado
    if (scanLedExt_) {
      scanLedExt_ = false;
    }
  }
  // Busca intermitente
  if (!c.extLedOn && c.intermittentScan) {
    intermittentLastScan_ = 0; // forçar primeira verificação
    intermittentCooldown_ = false;
    intermittentCooldownUntil_ = 0;
  }
}

void AppManager::blink(int times, LedMode next) {
  mode_      = LED_NBLINK;
  nextMode   = next;
  blinkLeft  = times;
  blinkStart = millis();
}

const char *AppManager::currentAppName() const {
  if (idx_ < 0 || idx_ >= APP_COUNT) return "?";
  return APP_NAMES[idx_];
}

const char *AppManager::ledPatternName() const {
  if (suspended_) {
    if (!ambient_) return "off";
    switch (ambientLevel_) {
      case 4:  return "on";    // colado
      case 3:  return "fast";  // perto
      case 2:  return "slow";  // médio
      default: return "blink"; // longe
    }
  }
  switch (mode_) {
    case LED_SOLID_OFF: return "off";
    case LED_SOLID_ON:  return "on";
    case LED_SLOW:      return "slow";
    case LED_FAST:      return "fast";
    case LED_NBLINK:    return "blink";
  }
  return "?";
}

String AppManager::stateString() const {
  String s = Config.get().deviceName;
  s += "|app:";
  s += idx_ + 1;
  s += "/";
  s += APP_COUNT;
  s += ",inapp:";
  s += inApp_ ? "1" : "0";
  s += ",led:";
  s += ledPatternName();
  return s;
}

// ------------------------------------------------------------
// Lógica do botão BOOT (novo comportamento)
// ------------------------------------------------------------
void AppManager::handleButton() {
  bool reading = (digitalRead(BTN_PIN) == LOW); // pressionado (ativo em baixo)

  if (reading != lastReading) {
    lastDebounce = millis();
    lastReading  = reading;
  }
  if (millis() - lastDebounce < BTN_DEBOUNCE_MS) return;

  stablePressed = reading;

  if (stablePressed && !prevPressed) {
    // aperto começou
    pressStart = millis();
    longFired  = false;
    prevPressed = true;
    lastPhys_   = millis();
    suspended_  = false;
    ambient_    = false;
  }

  if (stablePressed && !longFired && (millis() - pressStart >= BTN_LONG_MS)) {
    // aperto longo (>=600ms) -> toggle modo busca contínua
    longFired = true;
    if (searchMode_ == SEARCH_OFF) {
      setSearchMode(SEARCH_CONTINUOUS);
      blink(3, LED_SOLID_ON); // 3 piscadas = entrou no modo busca
    } else {
      setSearchMode(SEARCH_OFF);
      blink(3, LED_SOLID_ON); // 3 piscadas = saiu do modo busca
    }
  }

  if (!stablePressed && prevPressed) {
    prevPressed = false;
    if (!longFired) {
      // aperto curto (clique único) -> busca única
      if (searchMode_ == SEARCH_OFF) {
        setSearchMode(SEARCH_SINGLE);
      }
    }
  }
}

// ------------------------------------------------------------
// Modo de busca
// ------------------------------------------------------------
void AppManager::setSearchMode(SearchMode mode) {
  searchMode_ = mode;
  searchStart_ = millis();
  searchSpeakDone_ = false;
  if (mode == SEARCH_OFF) {
    mode_ = LED_SOLID_ON;
    bleScanner.stopScan();
    setBusy(false);
  } else if (mode == SEARCH_CONTINUOUS) {
    mode_ = LED_SEARCH;
    bleScanner.startScan();
    setBusy(true);
  } else if (mode == SEARCH_SINGLE) {
    mode_ = LED_SEARCH;
    bleScanner.startScan();
    setBusy(true);
    searchSpeakDone_ = false;
  }
}

// ------------------------------------------------------------
// Dispatcher de navegação (botão / web / BLE)
// ------------------------------------------------------------
void AppManager::selectApp() {
  inApp_ = true;
  blink(2, LED_SLOW); // 2 piscadas e depois piscar lento
}

void AppManager::dispatch(int action, int param) {
  switch (action) {
    case NAV_PREV:
      if (!inApp_) {
        idx_ = (idx_ + APP_COUNT - 1) % APP_COUNT;
        blink(idx_ + 1, LED_SOLID_ON); // LED pisca = posição do app
      }
      break;

    case NAV_NEXT:
      if (!inApp_) {
        idx_ = (idx_ + 1) % APP_COUNT;
        blink(idx_ + 1, LED_SOLID_ON);
      }
      break;

    case NAV_GOTO:
      if (!inApp_ && param >= 0 && param < APP_COUNT) {
        idx_ = param;
      }
      // continua para SELECT
      if (!inApp_) { selectApp(); }
      break;

    case NAV_SELECT:
      if (!inApp_) selectApp();
      break;

    case NAV_BACK:
      if (inApp_) {
        inApp_ = false;
        blink(idx_ + 1, LED_SOLID_ON);
      } else {
        idx_ = (idx_ + APP_COUNT - 1) % APP_COUNT;
        blink(idx_ + 1, LED_SOLID_ON);
      }
      break;

    case NAV_HOME:
      if (inApp_) {
        inApp_ = false;
        blink(idx_ + 1, LED_SOLID_ON);
      }
      break;
  }
}

void AppManager::ledConnecting(bool on) {
  if (on) blink(99, LED_FAST);  // fast enquanto conecta
  else    blink(1, LED_SOLID_ON);
}

void AppManager::ledReady() {
  mode_ = LED_SOLID_ON;   // aceso fixo, sem piscada extra
}

// Notificação pontual: entra numa fila e é tocada sem bloquear o loop
void AppManager::notify(int times, uint16_t onMs, uint16_t offMs) {
  if (times <= 0) return;
  if (notifyN_ >= NOTIFY_QMAX) return;
  notifyQ_[notifyN_].times = times;
  notifyQ_[notifyN_].onMs  = onMs ? onMs : 1;
  notifyQ_[notifyN_].offMs = offMs;
  notifyN_++;
}

// Modo radar BLE (só aparece enquanto suspenso)
void AppManager::setAmbient(bool active, uint8_t level) {
  ambient_      = active;
  ambientLevel_ = active ? level : 0;
}

// LED "ocupado" (double-blink): ESP32 trabalhando (BLE scan, WiFi, etc.)
void AppManager::setBusy(bool on) {
  if (on) {
    mode_ = LED_BUSY;
    busyUntil_ = millis() + 5000; // mínimo 5s de busy
  } else {
    if (mode_ == LED_BUSY) mode_ = LED_SOLID_ON;
    busyUntil_ = 0;
  }
}

// LED externo do radar BLE (2 pinos). Se habilitado, o radar ignora a
// suspensão e sempre reflete o radar BLE nesses pinos.
void AppManager::setScanLedPins(uint8_t posPin, uint8_t negPin) {
  scanLedExt_ = (posPin != 0) && (negPin != 0) && (posPin != negPin);
  scanLedPos_ = scanLedExt_ ? posPin : 0;
  scanLedNeg_ = scanLedExt_ ? negPin : 0;
  if (!scanLedExt_) return;
  pinMode(scanLedPos_, OUTPUT);
  pinMode(scanLedNeg_, OUTPUT);
  digitalWrite(scanLedNeg_, LOW);
  digitalWrite(scanLedPos_, LOW);
  ledcAttach(scanLedPos_, scanTone_hz, 8);
  ledcAttach(scanLedNeg_, scanTone_hz, 8);
}

// Escreve no LED externo de scan (pos/neg). O "neg" é o terminal comum do
// terra do LED; se inverter for verdadeiro, acende ao contrário.
void AppManager::writeScanLed(bool on) {
  if (!scanLedExt_) return;
  if (on) {
    ledcWriteTone(scanLedPos_, scanTone_hz);
    analogWrite(scanLedNeg_, 0);
  } else {
    ledcWriteTone(scanLedPos_, 0);
    analogWrite(scanLedNeg_, 0);
  }
}

void AppManager::writeLed(bool on) {
  int v = on ? brightness : 0;
  if (invert) v = on ? 0 : brightness;
  analogWrite(LED_PIN, v);
}

// ------------------------------------------------------------
// Atualização do LED (não bloqueante)
//   Prioridade: notificação > radar (suspenso) > suspenso > menu
// ------------------------------------------------------------
void AppManager::updateLed() {
  uint32_t now = millis();

  // inicia a próxima notificação da fila, se nenhuma estiver tocando
  if (oneshotLeft_ == 0 && notifyN_ > 0) {
    oneshotLeft_  = notifyQ_[0].times;
    oneshotOn_    = notifyQ_[0].onMs;
    oneshotOff_   = notifyQ_[0].offMs;
    oneshotStart_ = now;
    for (int i = 1; i < notifyN_; i++) notifyQ_[i - 1] = notifyQ_[i];
    notifyN_--;
  }

  // 1) notificação pontual
  if (oneshotLeft_ > 0) {
    uint32_t per     = (uint32_t)oneshotOn_ + oneshotOff_;
    uint32_t elapsed = now - oneshotStart_;
    if (elapsed >= (uint32_t)oneshotLeft_ * per) {
      oneshotLeft_ = 0;              // terminou -> segue para o resto
    } else {
      uint32_t t = elapsed % per;
      writeLed(t < oneshotOn_);
      return;
    }
  }

  // 2) radar BLE. Se houver LED externo de scan, ele SEMPRE reflete o radar
  //    (ignora suspensão). O LED embutido só faz radar enquanto suspenso.
  if (ambient_) {
    bool on = false;
    switch (ambientLevel_) {
      case 4: on = true;                             break; // colado: aceso
      case 3: on = ((now / 150ul) % 2ul) == 0ul;     break; // perto: rápida
      case 2: on = ((now / 400ul) % 2ul) == 0ul;     break; // médio
      case 1: on = ((now / 800ul) % 2ul) == 0ul;     break; // longe: longa
    }
    writeScanLed(on);                 // externo (noop se não habilitado)
    if (scanLedExt_) {
      // externo assume o radar: o interno segue para o padrão normal/menu
    } else if (suspended_) {
      writeLed(on);
      return;
    }
  }

  // 3) modo busca (SEARCH): pisca na distância do dispositivo mais próximo
  if (mode_ == LED_SEARCH) {
    if (ambient_) {
      bool on = false;
      switch (ambientLevel_) {
        case 4: on = true;                             break; // colado: aceso
        case 3: on = ((now / 150ul) % 2ul) == 0ul;     break; // perto: rápida
        case 2: on = ((now / 400ul) % 2ul) == 0ul;     break; // médio
        case 1: on = ((now / 800ul) % 2ul) == 0ul;     break; // longe: longa
      }
      writeLed(on);
      return;
    } else {
      // Sem alvo: pisca lento indicando busca ativa
      bool on = ((now / 1000ul) % 2ul) == 0ul;
      writeLed(on);
      return;
    }
  }

  // 3) suspenso sem radar: apagado
  if (suspended_) {
    writeLed(false);
    return;
  }

  // 4) LED_BUSY: double-blink rápido (ESP32 trabalhando)
  if (mode_ == LED_BUSY) {
    if (busyUntil_ > 0 && millis() >= busyUntil_) {
      mode_ = LED_SOLID_ON;   // timeout -> volta ao normal
    } else {
      // double-blink: 2 piscadas rápidas, pausa, repete
      bool on = false;
      uint32_t elapsed = millis() - blinkStart;
      if (elapsed >= 1000) { blinkStart = millis(); }
      on = (elapsed % 300) < 100 || (elapsed % 300) > 150 && (elapsed % 300) < 250;
      writeLed(on);
      return;
    }
  }

  // 5) padrão normal do menu
  bool on = false;
  if (mode_ == LED_NBLINK) {
    // sequência de N piscadas: 100ms ligado / 100ms apagado
    uint32_t elapsed = now - blinkStart;
    if (elapsed >= (uint32_t)blinkLeft * 200ul) {
      mode_ = nextMode;                 // terminou -> próximo estado
    } else {
      on = ((elapsed / 200ul) % 2ul) == 0ul;
    }
  }

  if (mode_ == LED_SOLID_ON)  on = true;
  else if (mode_ == LED_SOLID_OFF) on = false;
  else if (mode_ == LED_SLOW) on = ((now / 800ul) % 2ul) == 0ul;
  else if (mode_ == LED_FAST) on = ((now / 250ul) % 2ul) == 0ul;

  writeLed(on);
}

void AppManager::update() {
  handleButton();

  // ---- Modo de busca contínua (botão boot segurado) ----
  if (searchMode_ == SEARCH_CONTINUOUS) {
    if (!bleScanner.isScanning()) bleScanner.startScan();
    setBusy(true);
    int8_t rssi = bleScanner.nearestRssi(5000);
    if (rssi < 0) {
      uint8_t level = (rssi >= -30) ? 4 : (rssi >= -50) ? 3 : (rssi >= -70) ? 2 : 1;
      setAmbient(true, level);
      mode_ = LED_SEARCH;
    } else {
      setAmbient(false, 0);
      mode_ = LED_SEARCH;
    }
  }
  // ---- Modo busca única (clique curto) ----
  else if (searchMode_ == SEARCH_SINGLE) {
    if (!bleScanner.isScanning()) bleScanner.startScan();
    setBusy(true);
    int8_t rssi = bleScanner.nearestRssi(5000);
    if (rssi < 0 && !searchSpeakDone_) {
      searchSpeakDone_ = true;
      uint8_t level = (rssi >= -30) ? 4 : (rssi >= -50) ? 3 : (rssi >= -70) ? 2 : 1;
      setAmbient(true, level);
      mode_ = LED_SEARCH;
      searchStart_ = millis();
    } else if (searchSpeakDone_) {
      if (millis() - searchStart_ >= 3000) {
        speakWiFiNetworks();
        setSearchMode(SEARCH_OFF);
      }
    } else {
      mode_ = LED_SEARCH;
    }
  }
  // ---- Modo desligado: busca intermitente se configurado ----
  else if (searchMode_ == SEARCH_OFF) {
    DeviceConfig &c = Config.get();
    
    // Busca intermitente (quando LED externo desligado e habilitado)
    if (!c.extLedOn && c.intermittentScan) {
      uint32_t now = millis();
      uint16_t interval = c.intermittentIntervalMs ? c.intermittentIntervalMs : 30000;
      
      if (intermittentCooldown_) {
        if (now >= intermittentCooldownUntil_) {
          intermittentCooldown_ = false;
        }
      } else if (now - intermittentLastScan_ >= interval) {
        intermittentLastScan_ = now;
        if (!bleScanner.isScanning()) bleScanner.startScan();
        setBusy(true);
        
        static bool intermittentChecking = false;
        if (!intermittentChecking) {
          intermittentChecking = true;
        } else {
          intermittentChecking = false;
          int8_t rssi = bleScanner.nearestRssi(5000);
          if (rssi < 0) {
            uint8_t level = (rssi >= -30) ? 4 : (rssi >= -50) ? 3 : (rssi >= -70) ? 2 : 1;
            setAmbient(true, level);
            mode_ = LED_SEARCH;
            static uint32_t intermittentShowUntil = 0;
            intermittentShowUntil = millis() + 5000;
          } else {
            intermittentCooldown_ = true;
            intermittentCooldownUntil_ = now + 30000;
            intermittentLastScan_ = now;
          }
        }
      }
      
      static uint32_t intermittentShowUntil = 0;
      if (intermittentShowUntil > 0 && millis() >= intermittentShowUntil) {
        setAmbient(false, 0);
        intermittentShowUntil = 0;
      }
    }
    
    // Comportamento normal (radar BLE contínuo se scanLedExt ligado)
    if (bleScanner.isScanning() && !c.extLedOn) {
      if (searchMode_ == SEARCH_OFF) {
        bleScanner.stopScan();
        setBusy(false);
      }
    }
    
    // Update normal do radar BLE se scanLedExt ligado
    if (c.extLedOn && bleScanner.isScanning()) {
      int8_t rssi = bleScanner.nearestRssi(5000);
      if (rssi < 0) {
        uint8_t level = (rssi >= -30) ? 4 : (rssi >= -50) ? 3 : (rssi >= -70) ? 2 : 1;
        setAmbient(true, level);
        writeScanLed(true);
      } else {
        setAmbient(false, 0);
        writeScanLed(false);
      }
    }
  }

  // Suspensão automática
  if (!suspended_ && (millis() - lastPhys_ > SUSPEND_MS)) {
    suspended_ = true;
    ambient_   = false;
    oneshotLeft_ = 0;
  }
  updateWifiHunter();
  updateLed();
}

// Self-test: pisca LED embutido e LED externo (se configurado)
String AppManager::selfTest() {
  String result = "Self-test:\n";
  
  // Testa LED embutido
  result += "  LED embutido (GPIO" + String(LED_PIN) + "): ";
  for (int i = 0; i < 3; i++) {
    writeLed(true);
    delay(150);
    writeLed(false);
    delay(150);
  }
  result += "OK (3 piscadas)\n";
  
  // Testa LED externo se configurado
  if (scanLedExt_) {
    result += "  LED externo POS (GPIO" + String(scanLedPos_) + ") + NEG (GPIO" + String(scanLedNeg_) + "): ";
    for (int i = 0; i < 3; i++) {
      writeScanLed(true);
      delay(150);
      writeScanLed(false);
      delay(150);
    }
    result += "OK (3 piscadas)\n";
  } else {
    result += "  LED externo: não configurado\n";
  }
  
  result += "Fim do teste.\n";
  return result;
}

// Retorna bitmask de 32 bits com pinos reservados pelo sistema
uint32_t AppManager::systemUsedPins() const {
  uint32_t mask = 0;
  mask |= (1u << LED_PIN);      // LED embutido
  mask |= (1u << BTN_PIN);      // Botão BOOT
  if (scanLedExt_) {
    if (scanLedPos_ < 32) mask |= (1u << scanLedPos_);
    if (scanLedNeg_ < 32) mask |= (1u << scanLedNeg_);
  }
  // UART0 (Serial) - GPIO1 TX, GPIO3 RX
  mask |= (1u << 1);
  mask |= (1u << 3);
  return mask;
}

// GPIO: retorna JSON com estado de todos os pinos (0-39) + bitmask de reservados
String AppManager::gpioGetAll() {
  String json = "{";
  json += "\"reserved\":";
  json += String(systemUsedPins());
  json += ",\"pins\":[";
  
  for (int p = 0; p < 40; p++) {
    if (p > 0) json += ",";
    bool reserved = (systemUsedPins() & (1u << p)) != 0;
    int mode = 0; // 0=input, 1=output
    // Note: não há como ler o modo real no Arduino ESP32 facilmente
    // assumimos output se já foi configurado pelo sistema
    if (reserved && (p == LED_PIN || p == scanLedPos_ || p == scanLedNeg_)) mode = 1;
    int val = digitalRead(p);
    json += "{\"pin\":" + String(p) + ",\"mode\":" + String(mode) + ",\"val\":" + String(val) + ",\"reserved\":" + (reserved ? "true" : "false") + "}";
  }
  json += "]}";
  return json;
}

// GPIO: seta pino se não reservado. Retorna true se ok.
bool AppManager::gpioSet(uint8_t pin, bool on) {
  if (pin >= 40) return false;
  uint32_t reserved = systemUsedPins();
  if (reserved & (1u << pin)) return false; // pino reservado
  
  pinMode(pin, OUTPUT);
  digitalWrite(pin, on ? HIGH : LOW);
  return true;
}

// "Fala" redes WiFi: pisca LED para cada rede encontrada (curto = fraco, longo = forte)
void AppManager::speakWiFiNetworks() {
  // Para o scan BLE
  bleScanner.stopScan();
  setBusy(false);
  
  // Inicia scan WiFi
  WiFi.scanNetworks(true); // assíncrono
  
  // Aguarda scan terminar (máx 5s)
  uint32_t start = millis();
  int n = 0;
  while (millis() - start < 5000) {
    n = WiFi.scanComplete();
    if (n >= 0) break;
    delay(10);
  }
  
  if (n <= 0) {
    // Nenhuma rede - pisca 3x rápido
    for (int i = 0; i < 3; i++) {
      writeLed(true); delay(100);
      writeLed(false); delay(100);
    }
    return;
  }
  
  // Ordena por RSSI (mais forte primeiro)
  std::vector<std::pair<int, String>> nets;
  for (int i = 0; i < n; i++) {
    nets.emplace_back(WiFi.RSSI(i), WiFi.SSID(i));
  }
  std::sort(nets.begin(), nets.end(), [](auto &a, auto &b) { return a.first > b.first; });
  
  // "Fala" cada rede: pisca proporcional à força do sinal
  // RSSI forte (>= -50) = pisca longo (500ms), fraco (< -80) = pisca curto (100ms)
  for (auto &net : nets) {
    int rssi = net.first;
    int blinkTime = (rssi >= -50) ? 500 : (rssi >= -70) ? 300 : 150;
    
    writeLed(true);
    delay(blinkTime);
    writeLed(false);
    delay(200); // pausa entre redes
  }
  
  // Final: 3 piscadas rápidas = fim
  for (int i = 0; i < 3; i++) {
    writeLed(true); delay(80);
    writeLed(false); delay(80);
  }
}

// Morse code helper: play pattern on LED
// pattern: array of durations in ms, even indices = ON, odd = OFF
void AppManager::playMorse(const uint16_t *pattern, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    bool on = (i % 2 == 0);
    writeLed(on);
    delay(pattern[i]);
  }
  writeLed(false);
}

// Morse patterns (ms): even=ON, odd=OFF
// .-.. (L) = dot dash dot dot = 100,100, 300,100, 100,100, 100 = 7 elements
static const uint16_t MORSE_FOUND[] = {100,100, 300,100, 100,100, 100,100, 100,100, 100}; // .-.. + gap
// - - . (wait) = dash dash dot = 300,100, 300,100, 100 = 5 elements
static const uint16_t MORSE_WAITING[] = {300,100, 300,100, 100,100, 100,100, 100}; // - - . + gap
// . . - (connected) = dot dot dash = 100,100, 100,100, 300 = 5 elements
static const uint16_t MORSE_CONNECTED[] = {100,100, 100,100, 100,100, 300,100, 100,100, 100}; // . . - + gap
// . . (failed) = dot dot = 100,100, 100,100, 100 = 5 elements
static const uint16_t MORSE_FAILED[] = {100,100, 100,100, 100,100, 100,100, 100}; // . . + gap

void AppManager::triggerWifiHunterScan() {
  if (hunterState_ != HUNTER_IDLE) return;
  hunterState_ = HUNTER_SCANNING;
  hunterFound_ = false;
  Serial.println("[Hunter] Iniciando scan WiFi...");
}

// Update WiFi Hunter state machine
void AppManager::updateWifiHunter() {
  DeviceConfig &c = Config.get();
  if (!c.wifiHunter) {
    hunterState_ = HUNTER_IDLE;
    return;
  }

  uint32_t now = millis();
  uint16_t intervalMs = c.hunterInterval ? c.hunterInterval * 1000 : 30000;

  switch (hunterState_) {
    case HUNTER_IDLE:
      if (now - hunterLastScan_ >= intervalMs) {
        hunterState_ = HUNTER_SCANNING;
        hunterLastScan_ = now;
        hunterFound_ = false;
        Serial.println("[Hunter] Iniciando scan automático...");
      }
      break;

    case HUNTER_SCANNING:
      // Non-blocking scan
      if (!scanningInProgress_) {
        WiFi.scanNetworks(true);
        scanningInProgress_ = true;
        scanStartTime_ = now;
      } else if (WiFi.scanComplete() >= 0) {
        scanningInProgress_ = false;
        int n = WiFi.scanComplete();
        if (n > 0) {
          // Find best open network
          int bestIdx = -1;
          int bestRssi = -200;
          for (int i = 0; i < n; i++) {
            if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
              int rssi = WiFi.RSSI(i);
              if (rssi > bestRssi) {
                bestRssi = rssi;
                bestIdx = i;
              }
            }
          }
          if (bestIdx >= 0) {
            // Found open network!
            hunterFound_ = true;
            hunterFoundSsid_ = WiFi.SSID(bestIdx);
            hunterFoundRssi_ = WiFi.RSSI(bestIdx);
            hunterFoundTime_ = now;
            hunterState_ = HUNTER_WAITING;
            Serial.printf("[Hunter] Rede aberta encontrada: %s (RSSI %d)\n", hunterFoundSsid_.c_str(), hunterFoundRssi_);
            // Play .-.. (found)
            playMorse(MORSE_FOUND, sizeof(MORSE_FOUND)/sizeof(MORSE_FOUND[0]));
          } else {
            hunterState_ = HUNTER_IDLE;
          }
        } else {
          hunterState_ = HUNTER_IDLE;
        }
      } else if (now - scanStartTime_ > 10000) {
        // Scan timeout
        scanningInProgress_ = false;
        WiFi.scanDelete();
        hunterState_ = HUNTER_IDLE;
      }
      break;

    case HUNTER_WAITING:
      // Wait 3 seconds to see if network persists
      if (now - hunterFoundTime_ >= 3000) {
        // Re-scan to confirm
        WiFi.scanNetworks(true);
        scanningInProgress_ = true;
        scanStartTime_ = now;
        hunterState_ = HUNTER_CONNECTING;
      } else {
        // Play - - . (waiting) every 2 seconds
        static uint32_t lastMorse = 0;
        if (now - lastMorse >= 2000) {
          playMorse(MORSE_WAITING, sizeof(MORSE_WAITING)/sizeof(MORSE_WAITING[0]));
          lastMorse = now;
        }
      }
      break;

    case HUNTER_CONNECTING:
      if (WiFi.scanComplete() >= 0) {
        scanningInProgress_ = false;
        int n = WiFi.scanComplete();
        bool stillThere = false;
        for (int i = 0; i < n; i++) {
          if (WiFi.SSID(i) == hunterFoundSsid_ && WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
            stillThere = true;
            break;
          }
        }
        if (stillThere) {
          // Connect!
          hunterState_ = HUNTER_FAILED; // Will be set to IDLE after
          WiFi.begin(hunterFoundSsid_.c_str(), "");
          uint32_t connStart = now;
          while (WiFi.status() != WL_CONNECTED && millis() - connStart < 15000) {
            delay(100);
          }
          if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[Hunter] Conectado a %s\n", hunterFoundSsid_.c_str());
            playMorse(MORSE_CONNECTED, sizeof(MORSE_CONNECTED)/sizeof(MORSE_CONNECTED[0]));
            // Save as STA config
            DeviceConfig &cfg = Config.get();
            cfg.staSsid = hunterFoundSsid_;
            cfg.staPassword = "";
            Config.save();
            refreshFromConfig();
          } else {
            Serial.println("[Hunter] Falha ao conectar");
            playMorse(MORSE_FAILED, sizeof(MORSE_FAILED)/sizeof(MORSE_FAILED[0]));
          }
        } else {
          // Network disappeared
          playMorse(MORSE_FAILED, sizeof(MORSE_FAILED)/sizeof(MORSE_FAILED[0]));
        }
        hunterState_ = HUNTER_IDLE;
      } else if (now - scanStartTime_ > 10000) {
        scanningInProgress_ = false;
        WiFi.scanDelete();
        hunterState_ = HUNTER_IDLE;
      }
      break;

    case HUNTER_FAILED:
      hunterState_ = HUNTER_IDLE;
      break;
  }
}