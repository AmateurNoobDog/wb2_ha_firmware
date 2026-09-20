# ha_common — 共享模块

> [English](README_en.md) | **中文**

各 HA 固件项目(light/switch/radar_rd_01/433_gateway/usb_sw/tts)共用的基础设施,避免代码重复。

## 文件说明

| 文件 | 说明 |
|------|------|
| `blufi_app.c/h` | BLE BluFi 配网模块(支持自定义数据接收 HA IP) |
| `store.c/h` | WiFi 凭据 + 推送配置持久化(EasyFlash) |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `ha_mdns.c/h` | mDNS 服务发现(设备局域网自动发现) |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## 使用方法

各模块的 `Makefile` 中通过以下配置引入:

```makefile
INCLUDE_COMPONENTS += ha_common
INCLUDE_COMPONENTS += lwip_mdns
EXTRA_COMPONENT_DIRS += $(BL60X_SDK_PATH)/applications/home_assistant/ha_common
```

各模块无需再包含 `blufi_app`、`store`、`wifi_sta` 的源文件,直接链接 ha_common 即可。

## 组件功能

### blufi_app
BLE BluFi 配网,开机时若无 WiFi 配置则自动进入配网模式。支持接收自定义 JSON 数据配置推送目标,
格式: `{"tcp":{"addr":"192.168.1.100","port":9101}}`。

### store
基于 EasyFlash 的键值存储,持久化以下配置:
- WiFi 凭据(`ROUTER_SSID`/`ROUTER_PWD`)
- 推送目标(`HA_HOST`/`HA_PORT`,兼容旧版 `HA_IP` 键)
- TTS 参数(`TTS_VOLUME`/`TTS_SPEED`,仅 tts 模块使用)
- 重启计数(`BOOT_CNT`),连续重启 ≥3 次自动进入配网模式
- 提供 `store_wifi_clear()` 清除 WiFi 配置和 CLI 命令 `cfg_clear`

### wifi_sta
WiFi STA 连接管理,处理 WiFi 事件。

### ha_mdns
mDNS 服务注册,使设备在局域网内可通过 `{hostname}.local` 发现。
主机名格式: `and-{type}-{MAC后3字节}`（如 `and-light-1D94F1`）。
TXT 记录包含设备类型和名称。
