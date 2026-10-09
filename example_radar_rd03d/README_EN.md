[中文](README.md) | English

# example_radar_rd03d — Rd-03D_V2 multi-target mmWave radar firmware

Firmware for Ai-WB2 (BL602) + Ai-Thinker Rd-03D_V2 24GHz mmWave radar, with WiFi +
BLE BluFi provisioning and TCP JSON reporting, to bring multi-target coordinates
and speed into Home Assistant.

**Current version: 1.1.0**

## Hardware

| Function | Ai-WB2 pin | Rd-03D_V2 pin | Parameters |
|----------|-----------|----------------|------------|
| Radar UART | GPIO4 (TX) / GPIO5 (RX) | RXD / TXD | UART1, 256000 8N1 |
| Log console | GPIO16 (TX) / GPIO7 (RX) | USB serial adapter | UART0, 115200 8N1 |

> **Note**: `printf` logging is hard-bound to **UART0** (GPIO16/GPIO7@115200,
> bound at boot). The radar must use **UART1 + GPIO4/GPIO5** — never UART0, otherwise
> log output is driven into the radar RX at the radar baud rate.
>
> Logging here is written straight to UART0 through `uart_app.c` (`uart_logf()` for
> formatted output); `ha_device_t.log` is wired to `uart_log`, so TCP server logs come
> out of the console as well. `blog` is not used (`LOG_ENABLED_COMPONENTS` keeps only
> `ha_lib` as a fallback).

## Files

| File | Description |
|------|-------------|
| `main.c` | Entry point, WiFi/provisioning events, radar receive task, push throttling, `ha_device_t` registration |
| `uart_app.c/h` | Thin UART wrapper (copied from `example_notify_tts`): `uart_init()` sets up uart0 log port + uart1 radar port, `uart_log/uart_logf` write logs directly |
| `rd03d.c/h` | Rd-03D_V2 UART driver (byte-wise state machine frame parser, config commands; protocol in [通信.md](./通信.md)) |
| `rd03d_handler.c/h` | HA callbacks: `get_device` entity definitions, `get_state` states, `push_cfg` command, frame cache |
| `app_config.h` | Device configuration (UART / throttle / port / name) |

## Entities

Entity ID format: `{MAC 12 hex chars}_{3-digit seq}` — **10 entities**:

| Seq | Type | Name | Unit |
|-----|------|------|------|
| 001 | `binary_sensor` | Presence | — |
| 002 / 003 / 004 | `sensor` | Target1 X / Y / speed | mm / mm / cm/s |
| 005 / 006 / 007 | `sensor` | Target2 X / Y / speed | mm / mm / cm/s |
| 008 / 009 / 010 | `sensor` | Target3 X / Y / speed | mm / mm / cm/s |

Negative speed means the target moves toward the radar. `value` is `null` when the
target slot is empty.

## TCP protocol (v2)

Port: **9100**

### Get device info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"毫米波雷达","model":"Rd-03D_V2",
 "manufacturer":"Ai-Thinker","sw_version":"1.1.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"binary_sensor","name":"有人","icon":"mdi:motion-sensor"},
   {"id":"ACD8297A605D_002","type":"sensor","name":"目标1 X","icon":"mdi:axis-x-light","unit":"mm"},
   {"id":"ACD8297A605D_003","type":"sensor","name":"目标1 Y","icon":"mdi:axis-y-light","unit":"mm"},
   {"id":"ACD8297A605D_004","type":"sensor","name":"目标1 速度","icon":"mdi:speedometer","unit":"cm/s"}
 ],
 "offline_timeout":0}
```

`offline_timeout` is always **0**: this device does **not** use push-only mode — HA keeps
polling, because a target leaving is never pushed and only polling can reflect it.

### Get state

```json
{"cmd":"get_state"}
```

Response (two targets present, third slot empty):
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1},
  {"id":"ACD8297A605D_002","type":"sensor","value":159},
  {"id":"ACD8297A605D_003","type":"sensor","value":286},
  {"id":"ACD8297A605D_004","type":"sensor","value":0},
  {"id":"ACD8297A605D_005","type":"sensor","value":561},
  {"id":"ACD8297A605D_006","type":"sensor","value":4503},
  {"id":"ACD8297A605D_007","type":"sensor","value":-16},
  {"id":"ACD8297A605D_008","type":"sensor","value":null},
  {"id":"ACD8297A605D_009","type":"sensor","value":null},
  {"id":"ACD8297A605D_010","type":"sensor","value":null}
]}
```

### Commands

```json
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}   // set push target
```

## Push reporting (port 9101)

Pushing starts after the target is set with `push_cfg`. Rules:

- **Presence change**: pushed immediately as a single entity
  ```json
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1}
  ```
- **Target X/Y/speed**: one message per target, speed-adaptive throttling
  ```c
  interval = clamp(PUSH_MAX_MS / (1 + |speed|), PUSH_MIN_MS, PUSH_MAX_MS)
  ```
  Defaults `PUSH_MIN_MS=250`, `PUSH_MAX_MS=5000`: the faster the target moves, the more
  often it pushes (down to 250ms); a static target pushes at most every 5s. Unchanged
  values are not pushed.
  ```json
  {"entities":[
    {"id":"ACD8297A605D_002","type":"sensor","value":159},
    {"id":"ACD8297A605D_003","type":"sensor","value":286},
    {"id":"ACD8297A605D_004","type":"sensor","value":0}
  ]}
  ```
- **A disappearing target is not pushed** — HA picks it up via `get_state` polling.
  When the target reappears it is treated as first appearance and pushed immediately.

## Build

```bash
cd applications/home_assistant/example_radar_rd03d
make -j4
# output: build_out/example_radar_rd03d.bin
```

## Configuration

Everything lives in `example_radar_rd03d/example_radar_rd03d/app_config.h`:

```c
#define RD03D_UART_ID        1        // radar on UART1
#define RD03D_UART_TX_PIN    4        // WB2 TX -> radar RX
#define RD03D_UART_RX_PIN    5        // WB2 RX <- radar TX
#define RD03D_UART_BAUDRATE  256000   // radar default baud rate
#define RD03D_POWERON_DELAY_MS 2000   // wait for radar power-up

#define PUSH_MAX_MS  5000             // slowest push interval
#define PUSH_MIN_MS  250              // fastest push interval
#define RADAR_LOG_PERIOD_MS 0         // parsed-frame print period (ms), 0 = off

#define TCP_SERVER_PORT  9100
#define DEVICE_NAME      "毫米波雷达"
#define DEVICE_MODEL     "Rd-03D_V2"
```

## Provisioning

1. Power on 3 times in a row to enter BLE BluFi provisioning
2. Send WiFi credentials over Bluetooth
3. The device reboots, connects, then starts the TCP server and mDNS on got_ip

Use the CLI command `cfg_clear` to erase stored WiFi credentials.
