# switch — 3 路智能开关固件

> [English](README_en.md) | **中文**

基于 Ai-WB2（BL602）的 3 路继电器开关固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

**当前版本: 1.0.1**

## 硬件连接

| 通道 | GPIO | 说明 |
|------|------|------|
| 开关 1 | 3 | 继电器控制（默认高电平有效） |
| 开关 2 | 14 | 继电器控制 |
| 开关 3 | 17 | 继电器控制 |

- 继电器极性：`RELAY_ACTIVE_HIGH=1`（高电平吸合）
- 通道数：`SWITCH_COUNT=3`

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `switch_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回实体状态 |
| `relay.c/h` | 继电器底层驱动（GPIO 初始化/设置/读取） |
| `app_config.h` | 设备配置（引脚/端口/名称/极性） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"智能开关","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"switch","name":"开关1","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_002","type":"switch","name":"开关2","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_003","type":"switch","name":"开关3","icon":"mdi:toggle-switch"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":0},
  {"id":"ACD8297A605D_002","type":"switch","on":1},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

### 控制

```json
{"cmd":"set","id":"ACD8297A605D_001","on":1}       // 开启开关 1
{"cmd":"set","id":"ACD8297A605D_002","on":0}       // 关闭开关 2
```

### 推送

设备连接 HA 的 9101 端口，推送所有开关状态：
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":1},
  {"id":"ACD8297A605D_002","type":"switch","on":0},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

## 编译

```bash
cd applications/home_assistant/switch
make -j4
# 产物: build_out/switch.bin
```

## 配置

所有配置在 `switch/switch/app_config.h`：

```c
#define SWITCH_PINS        {3, 14, 17}       // 各通道 GPIO
#define SWITCH_COUNT       3                  // 通道数
#define RELAY_ACTIVE_HIGH  1                  // 高电平吸合
#define SWITCH_NAMES       {"开关1", "开关2", "开关3"}  // 通道名称

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "智能开关"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.0.1"
```
