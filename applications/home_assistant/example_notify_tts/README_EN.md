English | [中文](README.md)

# example_notify_tts — Text-to-Speech Firmware

Text-to-speech firmware based on Ai-WB2 (BL602) + external TTS module, supports WiFi + BLE BluFi provisioning + TCP JSON control.

**Current Version: 1.1.0**

## Hardware Connections

| Channel | GPIO | Baud Rate | Description |
|---------|------|-----------|-------------|
| UART0 Debug TX | 16 | 115200 | Debug serial port |
| UART0 Debug RX | 7 | 115200 | Debug serial port |
| UART1 TTS TX | 4 | 9600 | TTS module send |
| UART1 TTS RX | 11 | 9600 | TTS module receive |

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration |
| `tts_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `set_state` handles read/volume/speed commands |
| `tw_tts_app.c/h` | TTS speech synthesis protocol implementation (frame format `0xFD` + UTF-8) |
| `uart_app.c/h` | UART0/UART1 initialization and send/receive wrapper |
| `app_config.h` | Device configuration (pins/baud rate/parameter ranges) |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"Text-to-Speech","model":"Ai-WB2-TW-TTS","sw_version":"1.1.0",
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

```json
{"id":"ACD8297A605D_001","text":"Hello World"}         // TTS read
{"id":"ACD8297A605D_002","value":7}                // Set volume (0-9)
{"id":"ACD8297A605D_003","value":3}                // Set speed (0-9)
```

### Push

Device automatically pushes to HA's port 9101 when volume/speed changes:
```json
{"entities":[
  {"id":"ACD8297A605D_002","type":"number","value":7},
  {"id":"ACD8297A605D_003","type":"number","value":3}
]}
```

## Build

```bash
cd applications/home_assistant/example_notify_tts
make -j4
# Output: build_out/example_notify_tts.bin
```

## Configuration

All configuration in `example_notify_tts/example_notify_tts/app_config.h`:

```c
#define UART_TTS_TX_PIN     4           // TTS module TX
#define UART_TTS_RX_PIN     11          // TTS module RX
#define UART_TTS_BAUDRATE   9600

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "tts"
#define DEVICE_NAME        "Text-to-Speech"
#define DEVICE_MODEL       "Ai-WB2-TW-TTS"
#define DEVICE_SW_VERSION  "1.1.0"

#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
```

## Provisioning

1. Boot 3 times consecutively to enter BLE provisioning mode
2. Send WiFi credentials via Bluetooth connection
3. Device connects to WiFi after restart
