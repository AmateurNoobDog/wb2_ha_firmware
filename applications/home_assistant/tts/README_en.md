# tts — TTS Speech Synthesis Firmware

> **English** | [中文](README.md)

TTS speech synthesis firmware based on Ai-WB2 (BL602) + TW-TTS module, with WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.0.1**

## Hardware Connection

| Function | GPIO | Baud Rate | Description |
|----------|------|-----------|-------------|
| UART0 TX (Debug) | 16 | 115200 | Debug serial output |
| UART0 RX (Debug) | 7 | 115200 | Debug serial input |
| UART1 TX (TTS) | 4 | 9600 | Connects to TW-TTS module |
| UART1 RX (TTS) | 11 | 9600 | Connects to TW-TTS module |

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `tts_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns volume/speed state |
| `tw_tts_app.c/h` | TW-TTS speech synthesis driver (synthesize/set volume/set speed) |
| `uart_app.c/h` | UART driver (dual UART init/send/receive/print) |
| `app_config.h` | Device config (pins/port/name/entity definitions) |
| `bouffalo.mk` | BL602 SDK component build script |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"TTS","model":"Ai-WB2-TW-TTS","manufacturer":"AND-DIY","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"notify","name":"TTS","icon":"mdi:speaker-message"},
   {"id":"ACD8297A605D_002","type":"number","name":"Volume","icon":"mdi:volume-high","min":0,"max":9,"step":1},
   {"id":"ACD8297A605D_003","type":"number","name":"Speed","icon":"mdi:volume-vibrate","min":0,"max":9,"step":1}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_002","type":"number","value":5},
  {"id":"ACD8297A605D_003","type":"number","value":5}
]}
```

### Control

Speech synthesis (notify entity):
```json
{"cmd":"set","id":"ACD8297A605D_001","text":"Hello World"}
```

Set volume (number entity, range 0-9):
```json
{"cmd":"set","id":"ACD8297A605D_002","value":7}
```

Set speed (number entity, range 0-9):
```json
{"cmd":"set","id":"ACD8297A605D_003","value":5}
```

Volume and speed changes are automatically saved to EasyFlash and restored after reboot.

### Push

Device connects to HA's port 9101 and pushes volume/speed state:
```json
{"entities":[
  {"id":"ACD8297A605D_002","type":"number","value":7},
  {"id":"ACD8297A605D_003","type":"number","value":5}
]}
```

## TTS Chip Communication Protocol

TW-TTS module communicates via UART1 at 9600bps, frame format:

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0xFD` | Frame header |
| 1-2 | Length | Data length = text byte count + 2 (big-endian) |
| 3 | `0x01` | Command: speech synthesis |
| 4 | `0x04` | Encoding: GB2312/UTF-8 |
| 5+ | Text data | Text to synthesize |

## Build

```bash
cd applications/home_assistant/tts
make -j4
# Output: build_out/tts.bin
```

## Configuration

All config in `tts/tts/app_config.h`:

```c
// UART pins
#define UART_DEBUG_TX_PIN   16
#define UART_DEBUG_RX_PIN   7
#define UART_TTS_TX_PIN     4
#define UART_TTS_RX_PIN     11

// Device info
#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "tts"
#define DEVICE_NAME        "TTS"
#define DEVICE_MODEL       "Ai-WB2-TW-TTS"
#define DEVICE_MANUFACTURER "AND-DIY"
#define DEVICE_SW_VERSION  "1.0.1"

// TTS parameter ranges
#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
```

## Entity Type Description

This module introduces two new entity types:

- **`notify`**: Notification entity, accepts `text` field to trigger speech synthesis playback
- **`number`**: Numeric entity, accepts `value` field to set parameters (volume/speed), with min/max/step attributes
