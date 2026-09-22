English | [中文](README.md)

# Ai-Thinker WB2 Home Assistant Firmware Collection

Smart home application firmware based on [Ai-Thinker-WB2 (BL602)](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK.
Devices connect to the local network via WiFi and integrate with Home Assistant using a lightweight TCP JSON protocol
(companion integration: [`and_home`](https://gitee.com/AmateurNoobDog/and_home)).

**Current Version: 1.1.0**

## Project Structure

```
applications/home_assistant/
├── ha_lib/                       # Shared library: device-agnostic TCP JSON server + cJSON parser + push module
├── ha_common/                    # Common components: store(wifi/easyflash), wifi_sta, blufi_app(BLE provisioning), ha_mdns
├── demo_switch/                  # [Dev Board] 3-channel smart switch (Ai-WB2-12F-Kit, GPIO relay)
├── demo_light/                   # [Dev Board] RGB color light (Ai-WB2-12F-Kit, 3-channel PWM)
├── demo_sensor_dht20/            # [Dev Board] Temperature & humidity sensor (Ai-WB2-12F-Kit + DHT20, I2C)
├── example_usb_switch/           # [Module] Single-channel USB power switch (Ai-WB2-01S, IO4 active low)
├── example_event_gateway_433/    # [Module] 433MHz remote control gateway (Ai-WB2-12F + R1A, UART1 receiver)
├── example_binary_sensor_radar/  # [Module] Radar presence detection (RD-01, SPI + UART)
└── example_notify_tts/           # [Module] Text-to-speech (Ai-WB2-12F + TW-TTS, UART)
```

### ha_lib (Shared Components)
- `ha_device.h` — Device abstraction: type, name, model, version, port, `get_device`/`get_state`/`set_state` callbacks
- `ha_json.c/h` — cJSON-based JSON field parser (`ha_json_str`/`ha_json_int`)
- `tcp_json_server.c` — Generic TCP server: responds to `get_device`/`get_state`/`set` commands
- `ha_push.c/h` — Push module (TCP push to HA configured target IP/port)

New devices only need to implement their own handler and register `ha_device_t` to reuse the server.

### ha_common (Common Components)
- `store.h/c` — WiFi credential persistence (EasyFlash), BOOT_CNT consecutive reboot counter
- `wifi_sta.h/c` — WiFi STA mode startup, event-driven
- `blufi_app.h/c` — BLE BluFi provisioning, auto-restart after successful configuration
- `ha_mdns.h/c` — mDNS service registration (`_and._tcp`, port 9100, TXT: type/name)

## Usage

### 1. Clone into SDK

Based on the official [Ai-Thinker-WB2 SDK](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2):

```bash
git clone --recursive https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
cd Ai-Thinker-WB2/applications
git clone https://gitee.com/AmateurNoobDog/wb2_ha_firmware.git home_assistant
```

### 2. Build

Requires complete SDK environment (BL60X_SDK_PATH pointing to SDK root directory, including riscv toolchain):

```bash
cd applications/home_assistant/demo_switch   # or demo_light, demo_sensor_dht20, example_usb_switch, example_event_gateway_433, example_binary_sensor_radar, example_notify_tts
make -j4
# Output: build_out/demo_switch.bin
```

### 3. Flash

Direct serial flash (using demo_switch as example):

```bash
cd <SDK>/tools/flash_tool
./bflb_iot_tool-ubuntu --chipname=BL602 --baudrate=921600 --port=/dev/ttyUSB0 \
  --pt=<project>/img_conf/partition_cfg_2M.toml \
  --dts=<project>/img_conf/bl_factory_params_IoTKitA_40M.dts \
  --firmware=<project>/build_out/demo_switch.bin
```

### 4. Network Provisioning

Boot provisioning logic:
```
Boot → BOOT_CNT++
  │
  ├─ BOOT_CNT ≥ 3 → Clear BOOT_CNT → Enter BLE BluFi provisioning (preserve existing WiFi config)
  │
  ├─ WiFi config exists → Connect WiFi → got_ip → Clear BOOT_CNT → Normal operation
  │                                                  (TCP/mDNS/Push started)
  │
  └─ No WiFi config → Clear BOOT_CNT → Enter BLE BluFi provisioning
```

Device automatically restarts after successful provisioning and enters normal operation mode.

WiFi credentials are persisted via EasyFlash (`store.c`), can be cleared with CLI command `cfg_clear`.

## Device Protocol (TCP 9100)

Protocol version: **v2 (Entity-driven architecture)**

Request/response are single-line JSON, separated by `\n`.

### Get Device Info and Entity Definitions

```json
{"cmd":"get_device"}
```

Response example (switch):
```json
{"mac":"AC:D8:29:7A:60:5D","name":"Smart Switch","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"switch","name":"Switch 1","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_002","type":"switch","name":"Switch 2","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_003","type":"switch","name":"Switch 3","icon":"mdi:toggle-switch"}
 ]}
```

Response example (433 gateway):
```json
{"mac":"AC:D8:29:7A:60:5D","name":"433 Gateway","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"button","name":"Pair","icon":"mdi:remote","action":"pair"},
   {"id":"ACD8297A605D_002","type":"button","name":"Reset","icon":"mdi:restore","action":"reset"},
   {"id":"ACD8297A605D_003","type":"event","name":"Key Value","icon":"mdi:remote"}
 ]}
```

### Get Entity State

```json
{"cmd":"get_state"}
```

Response example:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":0},
  {"id":"ACD8297A605D_002","type":"switch","on":1},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

### Control Entity

```json
{"cmd":"set","id":"ACD8297A605D_002","on":1}       // Control switch 2
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":128}  // Control light
```

### General Commands

```json
{"cmd":"pair"}            // Pairing mode (433 gateway)
{"cmd":"reset"}           // Reset pairing
{"cmd":"calibrate"}       // Calibrate no presence (radar)
{"cmd":"restore"}         // Restore default parameters (radar)
```

### Device Push (Port 9101)

Device connects to HA's port 9101 and pushes entity states:

Single entity format (433/key_sensor):
```json
{"id":"ACD8297A605D_003","type":"event","event_type":"press","event_id":"A2B861"}
```

Multi-entity format (switch):
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":1},
  {"id":"ACD8297A605D_002","type":"switch","on":0}
]}
```

### Entity Type Matrix

| Type | Has Command | Has State | Description |
|------|-------------|-----------|-------------|
| `light` | set (r/g/b/brightness) | Yes | RGB light |
| `switch` | set (on) | Yes | Relay switch |
| `binary_sensor` | — | Yes | Binary sensor (radar motion/presence) |
| `button` | action cmd | — | Button (pair/reset) |
| `event` | — | Yes | Event (433 key press/release) |
| `sensor` | — | Yes | Data sensor (temperature, humidity, key value) |
| `notify` | text cmd | — | TTS text-to-speech |
| `number` | value cmd | | Numeric control (volume/speed) |

### Entity ID Format

```
{Full MAC hex (12 digits)}_{3-digit sequence number}
```

Example: `ACD8297A605D_001`, `ACD8297A605D_002`

### mDNS Auto Discovery

Device registers mDNS service after startup:
- Service type: `_and._tcp`
- Hostname: `and-{type}-{last 3 bytes of MAC}.local` (e.g., `and-light-1D94F1.local`)

Home Assistant automatically discovers devices on the local network via zeroconf.

## Configuration

- **demo_light**: Pin/channel configuration in `demo_light/demo_light/app_config.h` (`LED_*_PIN`)
- **demo_switch**: Relay pins, polarity, channel names in `demo_switch/demo_switch/app_config.h`
  (`SWITCH_PINS`, `RELAY_ACTIVE_HIGH`, `SWITCH_NAMES`)
- **example_usb_switch**: Single USB power switch pin in `example_usb_switch/example_usb_switch/app_config.h`
  (`SWITCH_PINS`=IO4, `RELAY_ACTIVE_HIGH`=0 active low)
- **example_event_gateway_433**: 433 remote receiver config in `example_event_gateway_433/example_event_gateway_433/app_config.h`
  (UART1: TX=GPIO6, RX=GPIO4, 9600bps; pair button: GPIO5)
- **example_notify_tts**: TTS config in `example_notify_tts/example_notify_tts/app_config.h`
  (UART1 TTS: TX=GPIO4, RX=GPIO11, 9600bps; volume/speed range 0-9)
- **demo_sensor_dht20**: Temperature & humidity sensor config in `demo_sensor_dht20/demo_sensor_dht20/app_config.h`
  (I2C: SCL=IO12, SDA=IO3, 100kHz; report interval 5s)
- **example_binary_sensor_radar**: Radar config in `example_binary_sensor_radar/example_binary_sensor_radar/app_config.h`
  - Debug switches: `RADAR_GATE_DATA_ENABLE` (gate data) / `RADAR_DEBUG_COUNTER_ENABLE` (counter)

## Dependencies

- Built on official [Ai-Thinker-WB2](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK;
- Makefile structure mirrors official `applications/bluetooth/blufi` example (consistent BLE provisioning environment);
- This repository contains only application code, not the SDK itself.

## Related Repositories

- Firmware repository: [amateur-dog/wb2_ha_firmware](https://gitee.com/amateur-dog/wb2_ha_firmware)
- Home Assistant integration: [AmateurNoobDog/and_home](https://gitee.com/AmateurNoobDog/and_home)
