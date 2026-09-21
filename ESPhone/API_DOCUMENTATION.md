# ESPhone API Documentation

Base URL: `http://<IP_DO_ESP32>/api`

## Authentication
Todas as rotas POST (exceto `/api/nav`) requerem autenticação via PIN (se configurado).
Header: `Authorization: Bearer <PIN>` ou query param `?pin=<PIN>`

---

## Endpoints

### Sistema

#### GET `/api/info`
Retorna informações do dispositivo.
**Response:**
```json
{
  "deviceName": "ESPhone",
  "version": "1.0.0",
  "chip": "ESP32-D0WD-V3",
  "revision": "v3.1",
  "cores": 2,
  "cpuFreq": 240,
  "flashSize": 4194304,
  "psram": true,
  "psramSize": 4194304,
  "temp": 32.5,
  "heapFree": 180000,
  "uptime": 3600
}
```

#### GET `/api/state`
Estado atual do dispositivo.
  "app": "1/8",
  "inapp": 0,
  "led": "on",
  "ip": "192.168.4.1"

#### POST `/api/reboot`
Reinicia o ESP32.
**Response:** `{"ok":true}`

#### POST `/api/reset`
Restaura configurações de fábrica.

#### POST `/api/self-test`
Executa auto-teste (LED embutido + externo).
{"ok":true,"result":"Self-test:\n  LED embutido (GPIO2): OK (3 piscadas)\n  LED externo POS (GPIO2) + NEG (GPIO0): OK (3 piscadas)\nFim do teste.\n"}


### Configurações

#### GET `/api/config`
Obtém todas as configurações.
  "theme": "auto",
  "accent": 2195676,
  "ledBrightness": 255,
  "ledInvert": false,
  "ledScanExt": false,
  "scanSpeakPosPin": 12,
  "scanSpeakNegPin": 13,
  "apEnabled": true,
  "apShareInternet": false,
  "apSsid": "ESPhone-A1B2",
  "apPassword": "esphone123",
  "staSsid": "MinhaRede",
  "staPassword": "senha123",
  "extLedOn": false,
  "extLedInvert": false,
  "extLedPosPin": 0,
  "extLedNegPin": 0,
  "intermittentScan": false,
  "intermittentIntervalMs": 30000,
  "wifiHunter": false,
  "hunterInterval": 30,
  "scanTone_hz": 2200,
  "uiPin": ""

#### POST `/api/config`
Atualiza configurações (parciais aceitas).
**Body:** (mesmo formato do GET, campos opcionais)

#### POST `/api/wifi`
Configura WiFi.
**Body:**
  "mode": "sta",
  "ssid": "MinhaRede",
  "password": "senha123"
Ou para AP:
  "mode": "ap",
  "ssid": "ESPhone-XXXX",
  "password": "esphone123"


### Navegação (Botão/APP)

#### POST `/api/nav`
Controla navegação do menu.
{"action": "next"}      // próximo app
{"action": "prev"}      // app anterior
{"action": "select"}    // entra no app
{"action": "back"}      // volta/sai do app
{"action": "home"}      // volta ao menu principal
{"action": "goto", "index": 2}  // vai para app índice 2

**Apps (índices):**
- 0: Configurações
- 1: Bluetooth
- 2: WiFi Monitor
- 3: Sobre
- 4: Sistema
- 5: Notas
- 6: Módulos
- 7: GPIO


### Bluetooth / BLE

#### GET `/api/ble/scan`
Lista dispositivos BLE encontrados.
**Query:** `?active=true` (inicia scan), `?active=false` (para scan)
  "scanning": true,
  "devices": [
    {
      "name": "ESPhone-A1B2",
      "addr": "AA:BB:CC:DD:EE:FF",
      "rssi": -45,
      "type": 1,
      "saved": false,
      "svc": ["00001800-0000-1000-8000-00805f9b34fb"]
    }
  ]

#### POST `/api/ble/scan`
Controla scan BLE.
**Body:** `{"active": true}` ou `{"active": false}`

#### GET `/api/ble/saved`
Lista dispositivos salvos.
    {"addr": "AA:BB:CC:DD:EE:FF", "name": "Meu ESP32"}

#### POST `/api/ble/save`
Salva dispositivo.
**Body:** `{"addr": "AA:BB:CC:DD:EE:FF", "name": "Meu ESP32"}`

#### POST `/api/ble/forget`
Remove dispositivo salvo.
**Body:** `{"addr": "AA:BB:CC:DD:EE:FF"}`

#### POST `/api/ble/gatt`
Conecta e descobre serviços GATT.
  "services": [
      "uuid": "00001800-0000-1000-8000-00805f9b34fb",
      "chars": [
        {
          "uuid": "00002a00-0000-1000-8000-00805f9b34fb",
          "props": ["read", "write"]
        }
      ]

#### POST `/api/ble/gatt/read`
Lê característica GATT.
**Body:** `{"addr": "...", "svc": "uuid", "chr": "uuid"}`
**Response:** `{"ok":true, "hex": "0100", "ascii": ".."}`

#### POST `/api/ble/gatt/write`
Escreve característica GATT.
**Body:** `{"addr": "...", "svc": "uuid", "chr": "uuid", "hex": "0100"}`

#### POST `/api/ble/gatt/notify`
Ativa/desativa notificações.
**Body:** `{"addr": "...", "svc": "uuid", "chr": "uuid", "enable": true}`

#### GET `/api/ble/gatt/status`
Status de notificações ativas.
  "on": true,
  "key": "uuid|uuid",
  "hex": "0100",
  "ascii": ".."


### WiFi Monitor

#### GET `/api/wifi/monitor`
Metadados do tráfego WiFi.
  "ch": 6,
  "pps": 150,
  "total": 5000,
  "beacon": 100,
  "probe": 50,
  "data": 200,
  "mgmt": 150,
  "ctrl": 20,
  "misc": 30,
  "rssi": -45,
  "rssiMin": -80,
  "rssiMax": -30,
  "ch": 6

#### POST `/api/wifi/monitor`
Controla monitor.
**Body:** `{"on": true, "hop": true}` ou `{"on": false}`

#### GET `/api/wifi/hunter`
Status do Caça-WiFi.
  "enabled": true,
  "interval": 30,
  "status": "waiting",
  "foundSsid": "RedeLivre",
  "foundRssi": -45

#### POST `/api/wifi/hunter`
Controla Caça-WiFi.
**Body:** `{"action": "scan"}` ou `{"action": "toggle"}`
**Response:** `{"ok":true, "wifiHunter": true}`


### Notas

#### GET `/api/notes`
Lê notas salvas.
**Response:** `"Minhas notas aqui..."`

#### POST `/api/notes`
Salva notas.
**Body:** `{"notes": "Texto das notas..."}`


### Motor / GPIO

#### GET `/api/motor/probe`
Detecta motor (back-EMF).
{"ok":true, "resolved": true, "transitions": 12, "peak": 2048}

#### POST `/api/pin/setup`
Configura pino GPIO.
**Body:** `{"pin": 2, "mode": "output", "value": 1}`

#### POST `/api/gpio`
Seta pino GPIO.
**Body:** `{"pin": 2, "value": 1}`

#### GET `/api/gpio`
Estado de todos os GPIOs.
  "reserved": 2147483647,
  "pins": [
    {"pin": 0, "mode": 0, "val": 0, "reserved": true},
    {"pin": 2, "mode": 1, "val": 1, "reserved": true}


### WebSocket (Tempo Real)

#### WS `/ws`
Conexão WebSocket para atualizações em tempo real.
**Mensagens recebidas:**
{"type": "state", "data": {...}}      // estado do dispositivo
{"type": "ble", "devices": [...]}     // dispositivos BLE
{"type": "wifi", "data": {...}}       // monitor WiFi
{"type": "hunter", "data": {...}}     // Caça-WiFi
{"type": "log", "msg": "..."}         // logs

**Enviar comandos:**
{"cmd": "PREV"}
{"cmd": "WIFI_SCAN"}


## Códigos de Erro

| Código | Significado |
|--------|-------------|
| 200 | OK |
| 400 | Requisição inválida |
| 401 | Não autorizado (PIN incorreto) |
| 404 | Não encontrado |
| 500 | Erro interno |


## Exemplo de Uso (JavaScript)

```javascript
const BASE = 'http://192.168.4.1/api';

// Conectar via BLE
async function connectBLE(device) {
  const response = await fetch(`${BASE}/nav`, {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({action: 'goto', index: 1}) // App Bluetooth
  });

// Scan BLE
async function scanBLE() {
  await fetch(`${BASE}/ble/scan`, {
    body: JSON.stringify({active: true})
  // Poll GET /api/ble/scan até scanning=false

// Enviar comando
async function sendCmd(cmd) {
  await fetch(`${BASE}/nav`, {
    body: JSON.stringify({action: cmd.toLowerCase()})

// WebSocket
const ws = new WebSocket('ws://192.168.4.1/ws');
ws.onmessage = (e) => {
  const msg = JSON.parse(e.data);
  if (msg.type === 'ble') updateBLEList(msg.devices);
};


## Configurações Importantes (NVS Keys)

| Chave | Descrição | Tipo |
|-------|-----------|------|
| `dname` | Nome do dispositivo | String |
| `theme` | Tema (auto/light/dark) | String |
| `accent` | Cor de destaque | Int (0xRRGGBB) |
| `ledbri` | Brilho LED (1-255) | UChar |
| `ledinv` | LED invertido | Bool |
| `scanx` | LED externo BLE | Bool |
| `scanp` | Pino POS externo | UChar |
| `scann` | Pino NEG externo | UChar |
| `apen` | AP habilitado | Bool |
| `apshr` | Compartilhar internet (NAT) | Bool |
| `apssid` | SSID do AP | String |
| `appass` | Senha do AP | String |
| `stassid` | SSID STA | String |
| `stapass` | Senha STA | String |
| `extlon` | LED externo ligado | Bool |
| `extlinv` | LED externo invertido | Bool |
| `extlpos` | Pino POS LED externo | UChar |
| `extlneg` | Pino NEG LED externo | UChar |
| `intscan` | Busca intermitente | Bool |
| `intint` | Intervalo intermitente (ms) | UShort |
| `wfhunt` | Caça-WiFi ativo | Bool |
| `huntint` | Intervalo Caça-WiFi (s) | UShort |
| `scanTone_hz` | Tom do beep (Hz) | Int |
| `uipin` | PIN de acesso | String |


## Fluxo Típico do App

1. **Onboarding** → Salva `onboarding_done=true` no SharedPreferences
2. **Permissões** → Android 12+ pede BLUETOOTH_CONNECT/SCAN + LOCATION; Android 11 pede só LOCATION
3. **Scan** → POST `/api/ble/scan` com `{"active":true}` → Poll GET `/api/ble/scan`
4. **Conectar** → User toca no dispositivo → App conecta via GATT (BLE) → Envia comandos via característica CMD (UUID `a101`)
5. **Conectado** → WebSocket `/ws` para updates real-time + REST para comandos


## UUIDs BLE (ESP32)

| Característica | UUID | Propriedades |
|----------------|------|--------------|
| Service | `0000a100-0001-4000-8000-00805f9b34fb` | - |
| CMD (Write) | `0000a101-0001-4000-8000-00805f9b34fb` | WRITE, READ |
| STATE (Notify) | `0000a102-0001-4000-8000-00805f9b34fb` | NOTIFY, READ |
| NAME (R/W) | `0000a103-0001-4000-8000-00805f9b34fb` | READ, WRITE |

**Comandos CMD:** `NEXT`, `PREV`, `SELECT`, `BACK`, `HOME`, `REBOOT`, `GOTO:n`
## Migração JSON (estágio 1-4 — concluído e COMPILANDO)

Contrato: o ESP32 fala JSON puro (protocol.h), serve /api/* (por LittleFS /apps /system /config /data) e BLE fala o MESMO JSON.

Novas camadas na raiz do sketch (compilam a 58% flash / 22% RAM, huge_app):
  - FileSystem.h/.cpp  (LittleFS: /system /apps /config /data, helpers JSON)
  - AppRegistry.h/.cpp (apps por ID + endpoints JSON por app)
  - Protocol.h/.cpp    (contrato JSON state/screen/action/led — BLE usa igual)
  - AppApi.h           (rotas /api/apps /api/app/<id>/state|action em JSON)

Ainda pendente (próximo estágio): migrar WebPortal.cpp de HTML → JSON
nativo (remover webpages.h 63KB) e PIN+pairing via BOOT+LED.

Fim da migração — dispositivo responde http://<ip>/api/* e BLE JSON.
