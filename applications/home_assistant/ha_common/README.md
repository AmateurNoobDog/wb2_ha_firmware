[English](README_EN.md) | 中文

# ha_common — Home Assistant 公共组件

所有 HA 固件项目共享的公共组件，提供 WiFi 配置持久化、WiFi STA 连接管理、BLE BluFi 配网和 mDNS 服务注册功能。

## 文件说明

| 文件 | 说明 |
|------|------|
| `store.h/c` | WiFi 配置持久化（EasyFlash），BOOT_CNT 连续重启计数，推送配置存储，TTS 参数存储 |
| `wifi_sta.h/c` | WiFi STA 模式启动，事件驱动连接管理 |
| `blufi_app.h/c` | BLE BluFi 配网模块，配网成功后自动重启 |
| `ha_mdns.h/c` | mDNS 服务注册（`_and._tcp`，端口 9100，TXT: type/name） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## API

### store 模块

```c
// WiFi 配置
void store_init(void);
bool store_wifi_load(store_wifi_t *cfg);
bool store_has_wifi(void);
void store_wifi_save_ssid(const uint8_t *ssid, int len);
void store_wifi_save_pwd(const uint8_t *pwd, int len);
void store_wifi_clear(void);

// 推送配置
bool store_push_load(store_push_t *cfg);
void store_push_save(const char *host, uint16_t port);
void store_push_clear(void);

// 重启计数
bool store_reboot_provision_check(int threshold);
void store_reboot_provision_clear(void);

// TTS 参数
bool store_tts_load(uint8_t *volume, uint8_t *speed);
void store_tts_save(uint8_t volume, uint8_t speed);
```

### wifi_sta 模块

```c
void wifi_sta_start(const char *ssid, const char *pwd);
```

### blufi_app 模块

```c
void blufi_app_start(void);
```

### ha_mdns 模块

```c
void ha_mdns_start(void);
```

## 数据结构

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
    char host[64];       // HA 主机：IP 地址或主机名（如 homeassistant.local）
    uint16_t port;
    uint8_t enabled;     // 1 表示已配置 HA 目标
} store_push_t;
```

## 使用说明

### WiFi 配置持久化

WiFi 凭据通过 EasyFlash 存储，支持以下操作：
- 保存 SSID 和密码
- 加载已保存的配置
- 检查是否有 WiFi 配置
- 清除 WiFi 配置

### BOOT_CNT 连续重启计数

用于检测设备是否需要进入配网模式：
- 每次启动 BOOT_CNT++
- 连续重启 3 次进入 BLE 配网模式
- 配网成功后清零 BOOT_CNT

### mDNS 服务注册

设备启动后自动注册 mDNS 服务：
- 服务类型：`_and._tcp`
- 主机名：`and-{type}-{MAC后3字节}.local`
- TXT 记录：type=设备类型，name=设备名称

Home Assistant 通过 zeroconf 自动发现局域网内的设备。
