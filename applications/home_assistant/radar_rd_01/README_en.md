# radar_rd_01 — Radar Presence Detection Firmware

> **English** | [中文](README.md)

Human presence detection firmware based on Ai-Thinker RD-01 module (BL602 + radar chip integrated).
The firmware runs on the BL602 inside the module, communicating with the radar chip via SPI/I2C/UART,
and reports motion/presence status to Home Assistant via WiFi + TCP JSON.

**Current Version: 1.0.1**

## Hardware Connection

### RD-01 Module Internal Interfaces

Internal connections between BL602 and the radar chip inside the module:

| Function | BL602 GPIO | Description |
|----------|------------|-------------|
| SPI CLK | 11 | SPI clock (slave, ~12.5MHz) |
| SPI MOSI | 0 | Master out slave in (radar → BL602) |
| SPI CS | 22 | Chip select (active-low) |
| I2C SCL | 14 | Register configuration |
| I2C SDA | 3 | Register configuration |
| UART1 TX | 16 | Command channel (256000 baud) |
| UART1 RX | 7 | Command channel |
| Power Control | 20 | PMOS switch (high = power off) |
| EN | 12 | Radar enable |
| REXT | 21 | External reference resistor |

### Workflow

1. BL602 controls the radar chip power via GPIO 20 (PMOS switch)
2. Radar chip boots and sends detection data as SPI master at ~12.5MHz
3. BL602 SPI slave receives data into queue (`FUNC_QUEUE_SIZE=34`)
4. Data processing task parses radar frames, extracts motion state
5. TCP server reports status to HA via 1s polling

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, `ha_device_t` registration, radar data callback |
| `radar_handler.c/h` | Device callbacks: `get_device` returns entity definitions, `get_state` returns motion/presence state |
| `app_config.h` | Device config (pins/port/name/debug switches) |
| `bouffalo.mk` | BL602 SDK component build script |
| `D103/` | RD-01 module radar chip driver library (original, unmodified) |
| `bodysense_lib/` | Human sensing algorithm library |
| `axk_factory/` | Factory test library |
| `hal_wifi/` | WiFi HAL layer |
| `axk_cfg/` | Configuration management library |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"Radar","model":"RD-01","manufacturer":"AND-DIY","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"binary_sensor","name":"Presence","icon":"mdi:motion-sensor"},
   {"id":"ACD8297A605D_002","type":"binary_sensor","name":"Motion","icon":"mdi:run-fast"},
   {"id":"ACD8297A605D_003","type":"button","name":"Calibrate Empty","icon":"mdi:cog-counterclockwise","action":"calibrate"},
   {"id":"ACD8297A605D_004","type":"button","name":"Restore Defaults","icon":"mdi:restore","action":"restore"}
 ]}
```

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":0},
  {"id":"ACD8297A605D_002","type":"binary_sensor","value":1}
]}
```

### Control Commands

```json
{"cmd":"calibrate"}       // Calibrate empty (calibrate threshold)
{"cmd":"restore"}         // Restore default parameters
{"cmd":"push_cfg","host":"192.168.1.100","port":9101}  // Configure push target (supports IP and hostname)
```

### Push

Device actively pushes motion/presence status to HA's port 9101:
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1},
  {"id":"ACD8297A605D_002","type":"binary_sensor","value":1}
]}
```

### Motion vs Presence

Radar chip outputs raw status values (0-3):
| Raw Value | Meaning | Motion | Presence |
|-----------|---------|--------|----------|
| 0 | No detection | 0 | 0 |
| 1 | BODYMOTION (moving) | 1 | 1 |
| 2 | STATICMotion (stationary) | 0 | 1 |
| 3 | BOTH_STATUS (moving + stationary) | 1 | 1 |

- **Presence**: Any detection result is considered present (raw values 1/2/3), uses raw value directly
- **Motion**: Has debounce logic — when radar reports stationary (raw value=2), requires `STATIONARY_CONFIRM_COUNT` (default 10) consecutive readings before reporting no motion; until then, keeps `motion=1`

Debounce mechanism: `radar_handler_set_motion()` is called on each radar status report. If raw value is 2, the stationary counter increments. Only after reaching the threshold does `radar_handler_get_motion()` return 0; any non-stationary state resets the counter.

### Debug Mode

When debug switches are enabled in `app_config.h`, responses include additional fields:

With `RADAR_GATE_DATA_ENABLE=1`:
```json
{"mac":"...","type":"radar","model":"RD-01",
 "motion":0,"on":0,
 "g0":12,"g1":34,"g2":56,"g3":78,"g4":90,"g5":12,"g6":34,"g7":56,
 "scnt":100,"mcnt":5}
```

| Field | Description |
|-------|-------------|
| `g0`-`g7` | 8 gate energy values (75cm per gate, 6m total) |
| `scnt` | `get_state` call counter |
| `mcnt` | Motion event counter |

With `RADAR_DEBUG_COUNTER_ENABLE=1` (no gate data):
```json
{"mac":"...","type":"radar","model":"RD-01",
 "motion":0,"on":0,"scnt":100,"mcnt":5}
```

## Build

```bash
cd applications/home_assistant/radar_rd_01
make -j6
# Output: build_out/radar_rd_01.bin (~589K)
```

## Configuration

All config in `radar_rd_01/radar_rd_01/app_config.h`:

```c
// RD-01 module hardware (internal interfaces)
#define RADAR_CS_PIN          22      // SPI chip select (connected to radar chip)
#define RADAR_POWER_PIN       25      // Power control (controls radar chip)

// Device identity
#define TCP_SERVER_PORT       9100
#define TCP_SERVER_STACK      4096
#define DEVICE_TYPE           "radar"
#define DEVICE_NAME           "Radar"
#define DEVICE_MODEL          "RD-01"
#define DEVICE_MANUFACTURER   "AND-DIY"
#define DEVICE_SW_VERSION     "1.0.1"

// Push config
#define HA_PUSH_DEFAULT_PORT  9101    // HA push listening port

// Stationary debounce: consecutive no-motion readings before declaring stationary
#define STATIONARY_CONFIRM_COUNT   10

// Debug switches
#define RADAR_GATE_DATA_ENABLE    0   // Gate data reporting (0=off)
#define RADAR_DEBUG_COUNTER_ENABLE 0  // Debug counter (0=off)
```

## BluFi Custom Data

Radar firmware supports receiving HA push target config via BluFi, format:
```json
{"ha_ip":"192.168.1.100","ha_port":9101}
```

## Known Limitations

- RD-01 module SPI slave clock is limited by BL602 SPI peripheral, max ~12.5MHz
- `spi_timeout_handle` is never created (always NULL), SPI timeout callback not enabled
- `timer_60ms_handle` is a one-shot timer (period 60ms) for radar data processing scheduling
- Push is implemented but reverted to 1s polling mode only
