# ESPhone 📱

Transforme um **ESP32** em um dispositivo móvel de verdade — com **tela (web)**, **apps**,
**navegação pelo botão BOOT + LED embutido** e **Bluetooth (BLE)**.

> O ESP32 vira um "celular" minúsculo: a tela é uma interface web que funciona **no celular e no PC**,
> os apps rodam no próprio ESP32, e você navega por eles tanto pela web quanto pelo botão físico.

---

## ✨ Recursos

| Recurso | Descrição |
| --- | --- |
| **Tela / UI web** | Interface responsiva (celular e PC) com horário, barra de status, grade de apps e temas claro/escuro |
| **Navegação física** | Botão **BOOT**: aperto curto troca de app, segurar abre/volta. O **LED embutido** pisca mostrando o app selecionado |
| **LED inteligente** | Apaga após 15 s sem toque físico (suspensão) e vira **radar BLE**: pisca conforme a distância dos dispositivos; notifica WiFi no boot |
| **Apps** | **Configurações**, **Bluetooth** (scan, radar, salvos e comandos GATT), **WiFi Monitor** (metadados de pacotes), **Sobre**, **Sistema** (diagnóstico ao vivo) e **Notas** |
| **WiFi** | Modo **Access Point** (sempre disponível) + **Cliente** (conecta na sua rede) |
| **Bluetooth BLE** | O ESP32 anuncia um serviço BLE: você lê o estado, envia comandos e muda o nome |
| **Persistência** | Tudo fica salvo na NVS (nome, WiFi, tema, notas...) |
| **Zero dependências** | Só precisa do **core ESP32 do Arduino IDE** — nenhuma biblioteca extra |

---

## 📦 Arquivos do projeto

```
ESPhone/
├── ESPhone.ino        ← arquivo principal (abrir no Arduino IDE)
├── Config.h/.cpp      ← configurações salvas na NVS (nome, WiFi, tema, LED)
├── AppManager.h/.cpp  ← "sistema operacional": apps + botão BOOT + LED
├── WebPortal.h/.cpp   ← WiFi (AP+STA) e servidor web + API REST
├── BleService.h/.cpp  ← servidor Bluetooth BLE
├── BleScanner.h/.cpp  ← scan BLE + dispositivos salvos + cliente GATT
├── WifiSniffer.h/.cpp ← monitor WiFi (modo promíscuo, apenas metadados)
└── webpages.h         ← interface web completa (embutida em flash)
```

---

## 🔧 Instalação (Arduino IDE)

1. Instale o **Arduino IDE 2.x**.
2. Em **Preferências → URLs adicionais para gerenciadores de placas**, adicione:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Em **Gerenciador de placas**, instale **esp32 by Espressif** (versão 2.0.x ou 3.x).
4. Abra o arquivo `ESPhone/ESPhone.ino`.
5. Selecione a placa **ESP32 Dev Module** (ou a sua) e a porta USB.
6. **Carregue** (Upload). Não segure o botão BOOT durante o reset/upload — ele entra em modo download.
7. Abra o **Serial Monitor** (115200 baud) para ver o IP.

**Requisitos:** apenas o core `esp32` do Arduino. Nenhuma biblioteca externa.

---

## 🚀 Primeiro uso

1. O ESP32 cria um WiFi chamado **`ESPhone-XXXX`** (senha **`esphone123`**).
2. Conecte seu **celular ou PC** nessa rede (ou deixe o ESP32 na mesma rede via app de Configurações).
3. Abra **`http://192.168.4.1`** no navegador.
4. Pronto — é essa a "tela" do seu dispositivo. Adicione ao home do celular ("Adicionar à tela inicial") e vira um app.

> Se o ESP32 estiver conectado como cliente na sua rede, a página também está disponível no IP STA (veja em Sobre/Sistema).

---

## ⌨️ Configurar o WiFi pelo Monitor Serial

O firmware **não tem nenhuma rede WiFi no código** — nada de SSID/senha pessoal no fonte.
A rede é definida em tempo de execução e fica salva na **NVS** (memória flash do ESP32).

Abra o **Monitor Serial** (115200 baud) e digite os comandos (ENTER no final):

| Comando | O que faz |
| --- | --- |
| `wifi <ssid> <senha>` | Conecta na sua rede e **salva na NVS** (ex.: `wifi MinhaRede senha123`) |
| `wifi off` | Desconecta e **apaga** a rede salva (fica só o Access Point) |
| `ap <ssid> <senha>` | Define o nome/senha do Access Point do ESPhone |
| `ap on` / `ap off` | Liga/desliga o Access Point |
| `name <nome>` | Muda o nome do dispositivo |
| `status` | Mostra a configuração atual e o IP |
| `reboot` | Reinicia |
| `reset` | Restaura as configurações de fábrica |
| `help` | Lista os comandos |

> Use **aspas** para SSID/senha com espaço: `wifi "Minha Rede" "senha com espaço"`.

No boot o log mostra de onde veio cada valor, por exemplo:
`[Config] nome=ESPhone  AP=ESPhone-2568  STA=Victor  tema=auto`.

---

## 🕹️ Navegação (web + botão físico)

| Ação | Botão BOOT | Dica na web |
| --- | --- | --- |
| Trocar de app | Aperto **curto** | botões ◀ / ▶ |
| Abrir app | **Segurar** (~0,6 s) | tocar no ícone ou no botão OK |
| Voltar / Home | Qualquer aperto dentro do app | botão ← Voltar / ⌂ |

**Sincronia:** a web e o físico refletem o mesmo estado. Se você navegar pelo botão,
a tela da web acompanha — e vice-versa.

### Padrões do LED embutido

| LED | Significado |
| --- | --- |
| Piscadas **1×, 2×, 3×…** | Posição do app (1º, 2º, 3º…) na tela inicial |
| Pisca **rápido** | Conectando no WiFi da sua casa (STA) |
| Aceso **contínuo** | Pronto / tela inicial |
| Pisca **lento** | Dentro de um app |

**Notificações de WiFi (só no boot/conexão):**

| LED | Significado |
| --- | --- |
| 2 piscadas rápidas | Encontrou redes WiFi por perto |
| 1 piscada | Encontrou a rede conhecida (salva) |
| 3 piscadas | Conectou na rede conhecida |

**Suspensão + radar BLE:** se você não tocar no **botão físico** por **15 s** (navegar
pela web não conta), o LED apaga e entra em suspensão. Nesse modo o ESP continua
escaneando Bluetooth e usa o LED como "radar" de proximidade:

| Distância (RSSI) | LED |
| --- | --- |
| Longe (≤ −78 dBm) | Piscada **longa** (lenta) |
| Médio (−78 a −65 dBm) | Piscada **média** |
| Perto (−65 a −50 dBm) | Piscada **rápida** |
| Muito perto (> −50 dBm) | **Aceso** fixo |

Se não encontrar nada por ~4 s, o scan pausa **30 s** para economizar bateria (e
reinicia depois). Ao apertar o botão físico o LED acorda e volta ao menu.

> Se a sua placa tem o LED em outro pino, ou o LED "pisca ao contrário", ajuste
> `LED_PIN` / `BTN_PIN` no topo de `AppManager.cpp` e/ou ative **"LED invertido"** no app de Configurações.

---

## 📱 Apps

- **Configurações** — nome do dispositivo, tema (auto/claro/escuro), cor de destaque,
  brilho do LED, Access Point (ligar/desligar, SSID, senha), conectar na sua rede WiFi (STA),
  reiniciar e restaurar de fábrica.
- **Bluetooth** — escaneia dispositivos BLE por perto com abas **Dispositivos**, **Radar**
  e **Meus** (salvos na NVS). Cada dispositivo tem uma tela de detalhes com **Salvar**,
  **Esquecer** e **Comandos GATT** (ler, escrever em hex e assinar notificações).
- **WiFi Monitor** — mostra **apenas metadados** dos pacotes 802.11 (contadores, canal,
  RSSI, redes por beacon/SSID e endereços MAC). Não captura payload nem senhas.
  Opcionalmente faz varredura de canais 1–13 (pode derrubar a conexão STA).
- **Sobre** — chip, núcleos, frequência, flash/PSRAM, temperatura, endereços MAC, IP, versão.
- **Sistema** — diagnóstico **ao vivo** (memória livre, maior bloco, temperatura, uptime, WiFi, BLE) com atualização automática a cada 2 s.
- **Notas** — bloco de notas de 2000 caracteres salvo na memória do ESP32.

---

## 🔵 Bluetooth (BLE)

O ESP32 anuncia automaticamente o serviço `ESPhone`:

| Característica (UUID) | Propriedade | Descrição |
| --- | --- | --- |
| `0000a100-0001-4000-8000-00805f9b34fb` | serviço | Serviço ESPhone |
| `0000a101-…` | write | **Comandos**: `NEXT`, `PREV`, `SELECT`, `BACK`, `HOME`, `GOTO:<n>`, `REBOOT` |
| `0000a102-…` | notify/read | **Estado**: `Nome|app:2/6,inapp:0,led:on,ip:192.168.4.1` |
| `0000a103-…` | read/write | **Nome do dispositivo** (muda o nome no BLE e na web) |

Teste com o app **nRF Connect** ou **LightBlue**: faça scan, conecte no `ESPhone` e
escreva `NEXT` na característica de comandos (primeiro habilite "Notify" na de estado).

---

## 🌐 API HTTP (opcional)

| Rota | Método | Descrição |
| --- | --- | --- |
| `/` | GET | Interface web |
| `/api/state` | GET | Estado atual (app, LED, WiFi, BLE) |
| `/api/info` | GET | Diagnóstico do sistema |
| `/api/config` | GET/POST | Ler / salvar configurações |
| `/api/wifi` | POST | `{"mode":"sta","ssid":"…","password":"…"}` ou `"ap"` |
| `/api/nav` | POST | `{"action":"next"}` / `"prev"` / `"select"` / `"back"` / `"home"` / `{"action":"goto","index":2}` |
| `/api/notes` | GET/POST | Bloco de notas |
| `/api/ble/scan` | GET/POST | Lista dispositivos BLE (`{"active":true}` inicia/para o scan) |
| `/api/ble/saved` | GET | Dispositivos salvos na NVS |
| `/api/ble/save` | POST | `{"addr":"…","name":"…"}` salva dispositivo |
| `/api/ble/forget` | POST | `{"addr":"…"}` remove dispositivo salvo |
| `/api/ble/gatt` | POST | `{"addr":"…"}` conecta e lista serviços/características |
| `/api/ble/gatt/read` | POST | `{"addr":"…","svc":"…","chr":"…"}` lê característica |
| `/api/ble/gatt/write` | POST | `{"addr":"…","svc":"…","chr":"…","hex":"01A0"}` escreve |
| `/api/ble/gatt/notify` | POST | `{"addr":"…","svc":"…","chr":"…","enable":true}` assina/desassina |
| `/api/ble/gatt/status` | GET | Estado do cliente de notificação e último valor |
| `/api/wifi/monitor` | GET/POST | Estado do monitor e `{"on":true,"hop":false}` |
| `/api/reboot` | POST | Reinicia o dispositivo |
| `/api/reset` | POST | Restaura configurações de fábrica |

---

## 🧰 Ajustes por placa

A maioria das placas (ESP32 DevKit, NodeMCU-32S…) usa:
- **LED embutido** → `GPIO 2` (ativo em nível ALTO)
- **Botão BOOT** → `GPIO 0` (ativo em nível BAIXO)

Se a sua placa for diferente (ou não tiver LED), edite no topo de `AppManager.cpp`:

```cpp
#define LED_PIN  2   // ← pino do LED embutido
#define BTN_PIN  0   // ← pino do botão BOOT
```

---

## ⚠️ Dicas

- Não segure o botão **BOOT** ao ligar/resetar — isso coloca o ESP32 em modo download.
- O Access Point fica ligado por padrão para a tela estar sempre acessível (mesmo quando o ESP32 está conectado à sua rede).
- O serial monitor mostra os logs: IPs, comandos BLE recebidos e estado da conexão WiFi.

---

Feito com ❤️ para virar um gadget de bancada divertido. Divirta-se! 🛰️