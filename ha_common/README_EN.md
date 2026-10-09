English | [中文](README.md)

# ha_common — Home Assistant Common Components

Shared common components for all HA firmware projects, providing WiFi configuration persistence, WiFi STA connection management, BLE BluFi provisioning, and mDNS service registration.

## File Description

| File | Description |
|------|-------------|
| `store.h/c` | WiFi credential persistence (EasyFlash), BOOT_CNT consecutive reboot counter, push config storage, TTS parameter storage |
| `wifi_sta.h/c` | WiFi STA mode startup, event-driven connection management |
| `blufi_app.h/c` | BLE BluFi provisioning module, auto-restart after successful configuration |
| `ha_mdns.h/c` | mDNS service registration (`_and._tcp`, port 9100, TXT: type/name) |
| `bouffalo.mk` | BL602 SDK component build script |

## API

### store Module

```c
// WiFi configuration
void store_init(void);
bool store_wifi_load(store_wifi_t *cfg);
bool store_has_wifi(void);
void store_wifi_save_ssid(const uint8_t *ssid, int len);
void store_wifi_save_pwd(const uint8_t *pwd, int len);
void store_wifi_clear(void);

// Push configuration
bool store_push_load(store_push_t *cfg);
void store_push_save(const char *host, uint16_t port);
void store_push_clear(void);

// Reboot counter
bool store_reboot_provision_check(int threshold);
void store_reboot_provision_clear(void);

// TTS parameters
bool store_tts_load(uint8_t *volume, uint8_t *speed);
void store_tts_save(uint8_t volume, uint8_t speed);
```

### wifi_sta Module

```c
void wifi_sta_start(const char *ssid, const char *pwd);
```

### blufi_app Module

```c
void blufi_app_start(void);
```

### ha_mdns Module

```c
void ha_mdns_start(void);
```

## Data Structures

### store_wifi_t

```c
typedef struct {
    char ssid[64];
    char pwd[64];
} store_wifi_t;
```

### store_push_t

```c
typedef struct {
    char host[64];       // HA host: IP address or hostname (e.g. homeassistant.local)
    uint16_t port;
    uint8_t enabled;     // 1 if HA target is configured
} store_push_t;
```

## Usage

### WiFi Credential Persistence

WiFi credentials are stored via EasyFlash, supporting the following operations:
- Save SSID and password
- Load saved configuration
- Check if WiFi configuration exists
- Clear WiFi configuration

### BOOT_CNT Consecutive Reboot Counter

Used to detect if the device needs to enter provisioning mode:
- BOOT_CNT++ on each boot
- Enter BLE provisioning mode after 3 consecutive reboots
- Clear BOOT_CNT after successful provisioning

### mDNS Service Registration

Device automatically registers mDNS service after startup:
- Service type: `_and._tcp`
- Hostname: `and-{type}-{last 3 bytes of MAC}.local`
- TXT record: type=device type, name=device name

Home Assistant automatically discovers devices on the local network via zeroconf.
