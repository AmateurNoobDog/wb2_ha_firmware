# switch — 3 路智能开关固件

基于 Ai-WB2(BL602) 的 3 路继电器开关固件,支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

## 硬件连接

| 通道 | GPIO | 说明 |
|------|------|------|
| 开关 1 | 3 | 继电器控制(默认高电平有效) |
| 开关 2 | 14 | 继电器控制 |
| 开关 3 | 17 | 继电器控制 |

- 继电器极性:`RELAY_ACTIVE_HIGH=1`(高电平吸合)
- 通道数:`SWITCH_COUNT=3`

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口,WiFi 事件处理,`ha_device_t` 注册 |
| `switch_handler.c/h` | 设备回调: `get_state` 返回 `"model","count","names","on","on1","on2"` |
| `relay.c/h` | 继电器底层驱动(GPIO 初始化/设置/读取) |
| `store.c/h` | WiFi 凭据持久化(EasyFlash) |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `blufi_app.c/h` | BLE BluFi 配网模块 |
| `app_config.h` | 设备配置(引脚/端口/名称/极性) |

## TCP 协议

端口: **9100**

查询:
```json
{"cmd":"get"}
```

响应:
```json
{"mac":"AC:D8:29:7A:60:5D","type":"sw","name":"智能开关","model":"Ai-WB2-12F",
 "count":3,"names":["开关1","开关2","开关3"],
 "on":0,"on1":0,"on2":0}
```

控制:
```json
{"cmd":"set","on":1}        // 开启通道 1
{"cmd":"set","on1":0}       // 关闭通道 2
{"cmd":"set","on2":1}       // 开启通道 3
```

支持同时控制多通道:
```json
{"cmd":"set","on":1,"on1":1,"on2":0}
```

## 字段说明

| 字段 | 说明 |
|------|------|
| `count` | 继电器通道数(由 `SWITCH_COUNT` 定义) |
| `names` | 各通道名称数组(由 `SWITCH_NAMES` 定义) |
| `on` | 通道 1 状态(0=关/1=开) |
| `on1` | 通道 2 状态 |
| `on2` | 通道 3 状态 |

> 通道字段命名规则:第 1 通道为 `on`,第 2 通道起为 `on1`,`on2`,以此类推。

## 编译

```bash
cd applications/home_assistant/switch
make -j4
# 产物: build_out/switch.bin
```

## 配置

所有配置在 `switch/switch/app_config.h`:

```c
#define SWITCH_PINS        {3, 14, 17}       // 各通道 GPIO
#define SWITCH_COUNT       3                  // 通道数
#define RELAY_ACTIVE_HIGH  1                  // 高电平吸合
#define SWITCH_NAMES       {"开关1", "开关2", "开关3"}  // 通道名称

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "sw"
#define DEVICE_NAME        "智能开关"
#define DEVICE_MODEL       "Ai-WB2-12F"
```
