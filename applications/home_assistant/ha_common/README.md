# ha_common — 共享模块

各 HA 固件项目(light/switch/radar_rd_01)共用的基础设施,避免代码重复。

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
BLE BluFi 配网,开机时若无 WiFi 配置则自动进入配网模式。支持接收自定义 JSON 数据(如 HA 推送目标 IP)。

### store
基于 EasyFlash 的键值存储,持久化 WiFi 凭据(`ROUTER_SSID`/`ROUTER_PWD`)和推送配置(`HA_IP`/`HA_PORT`)。

### wifi_sta
WiFi STA 连接管理,处理 WiFi 事件并回调通知上层。

### ha_mdns
mDNS 服务注册,使设备在局域网内可通过 `{device_name}.local` 发现。
