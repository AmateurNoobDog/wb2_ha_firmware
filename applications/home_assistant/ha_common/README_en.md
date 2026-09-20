# ha_common — Shared Components

Shared infrastructure used by all HA firmware projects (light/switch/radar_rd_01/433_gateway/usb_sw/tts), avoiding code duplication.

## File Description

| File | Description |
|------|-------------|
| `blufi_app.c/h` | BLE BluFi provisioning module (supports custom data for HA push target) |
| `store.c/h` | WiFi credentials + push config persistence (EasyFlash) |
| `wifi_sta.c/h` | WiFi STA connection management |
| `ha_mdns.c/h` | mDNS service discovery (automatic LAN device discovery) |
| `bouffalo.mk` | BL602 SDK component build script |

## Usage

Include via the following configuration in each module's `Makefile`:

```makefile
INCLUDE_COMPONENTS += ha_common
INCLUDE_COMPONENTS += lwip_mdns
EXTRA_COMPONENT_DIRS += $(BL60X_SDK_PATH)/applications/home_assistant/ha_common
```

Modules do not need to include `blufi_app`, `store`, or `wifi_sta` source files — just link against ha_common.

## Component Functions

### blufi_app
BLE BluFi provisioning. If no WiFi config is present at boot, automatically enters provisioning mode. Supports receiving custom JSON data to configure push target,
format: `{"tcp":{"addr":"192.168.1.100","port":9101}}`.

### store
EasyFlash-based key-value storage, persisting the following:
- WiFi credentials (`ROUTER_SSID`/`ROUTER_PWD`)
- Push target (`HA_HOST`/`HA_PORT`, with legacy `HA_IP` key migration)
- TTS parameters (`TTS_VOLUME`/`TTS_SPEED`, tts module only)
- Reboot counter (`BOOT_CNT`), auto-enters provisioning mode after 3+ consecutive reboots
- Provides `store_wifi_clear()` to clear WiFi config and CLI command `cfg_clear`

### wifi_sta
WiFi STA connection management, handles WiFi events.

### ha_mdns
mDNS service registration, making the device discoverable on the LAN via `{hostname}.local`.
Hostname format: `and-{type}-{last 3 bytes of MAC}` (e.g., `and-light-1D94F1`).
TXT records contain device type and name.
