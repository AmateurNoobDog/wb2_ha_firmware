English | [中文](README.md)

# demo_sensor_dht20 — DHT20 Temperature & Humidity Sensor Firmware

Temperature & humidity sensor firmware based on Ai-WB2 (BL602) + DHT20, supports WiFi + BLE BluFi provisioning + TCP JSON reporting.

**Current Version: 1.1.0**

## Hardware Connections

| Function | GPIO | Description |
|----------|------|-------------|
| I2C SCL | 12 | DHT20 clock line |
| I2C SDA | 3 | DHT20 data line |

- I2C frequency: 100kHz
- Power supply: 3.3V

## File Description

| File | Description |
|------|-------------|
| `main.c` | Application entry, WiFi event handling, sensor read task, `ha_device_t` registration |
| `dht20.c/h` | DHT20 sensor driver (I2C communication, CRC8 verification, data conversion) |
| `app_config.h` | Device configuration (I2C pins/ports/names/report interval) |

## TCP Protocol (v2)

Port: **9100**

### Get Device Info

```json
{"cmd":"get_device"}
```

Response:
```json
{"mac":"AC:D8:29:7A:60:5D","name":"Temperature & Humidity Sensor","model":"Ai-WB2/DHT20","sw_version":"1.1.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"sensor","name":"Temperature","icon":"mdi:thermometer","device_class":"temperature","unit":"°C"},
   {"id":"ACD8297A605D_002","type":"sensor","name":"Humidity","icon":"mdi:water-percent","device_class":"humidity","unit":"%"}
 ],
 "offline_timeout":300}
```

`offline_timeout` (seconds): reports `300` only when a push target is configured, otherwise `0`.
When HA receives `>0` it enters **push-only mode** (no polling) and marks entities unavailable
if no push arrives within this timeout.

### Get State

```json
{"cmd":"get_state"}
```

Response:
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"sensor","value":29.5},
  {"id":"ACD8297A605D_002","type":"sensor","value":58.9}
]}
```

### Push

Device reads DHT20 every 5 seconds and pushes to HA's port 9101 when temperature or
humidity changes ≥0.1; it also forces a heartbeat push every 60 seconds (even if values
are unchanged) so HA does not falsely mark the device offline:
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"sensor","value":29.5},
  {"id":"ACD8297A605D_002","type":"sensor","value":58.9}
]}
```

## Build

```bash
cd applications/home_assistant/demo_sensor_dht20
make -j4
# Output: build_out/demo_sensor_dht20.bin
```

## Configuration

All configuration in `demo_sensor_dht20/demo_sensor_dht20/app_config.h`:

```c
#define DHT20_I2C_SCL_PIN       12
#define DHT20_I2C_SDA_PIN       3
#define DHT20_I2C_FREQ          100000

#define SENSOR_READ_INTERVAL_MS  5000
#define DEVICE_OFFLINE_TIMEOUT   300   // Offline timeout reported to HA (seconds), only when push is configured
#define PUSH_HEARTBEAT_MS        60000 // Push heartbeat (milliseconds)

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "sensor"
#define DEVICE_NAME        "Temperature & Humidity Sensor"
#define DEVICE_MODEL       "Ai-WB2/DHT20"
#define DEVICE_SW_VERSION  "1.1.0"
```

## Provisioning

1. Boot 3 times consecutively to enter BLE provisioning mode
2. Send WiFi credentials via Bluetooth connection
3. Device connects to WiFi after restart
