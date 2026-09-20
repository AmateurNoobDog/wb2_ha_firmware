# 433_gateway — 433 遥控网关固件

433MHz 遥控接收器网关固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制 + Push 推送。

**当前版本: 1.0.1**

## 硬件连接

- 模块：Ai-WB2-12F（BL602）
- 433MHz 接收器：UART1（TX=GPIO6, RX=GPIO4, 9600bps）
- 配对/重置按钮：GPIO5

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `event_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回事件状态，push 上报按键事件 |
| `uart_433.c/h` | UART 433 接收驱动 |
| `app_config.h` | 设备配置（引脚/端口/名称） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"7C:B9:4C:D1:F7:67","name":"433网关","model":"Ai-WB2-12F","sw_version":"1.0.1",
 "entities":[
   {"id":"7CB94CD1F767_001","type":"button","name":"配对","icon":"mdi:remote","action":"pair"},
   {"id":"7CB94CD1F767_002","type":"button","name":"重置","icon":"mdi:restore","action":"reset"},
   {"id":"7CB94CD1F767_003","type":"event","name":"键值","icon":"mdi:remote"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应（无按键事件时返回空 entities）：
```json
{"state":"online","entities":[]}
```

有按键事件时：
```json
{"state":"online","entities":[
  {"id":"7CB94CD1F767_003","type":"event","event_type":"release","event_id":"A2B861"}
]}
```

### 控制命令

```json
{"cmd":"pair"}            // 进入配对模式
{"cmd":"reset"}           // 重置配对
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}  // 配置推送目标
```

### 推送（端口 9101）

设备主动推送按键事件到 HA 的 9101 端口：

```json
{"id":"7CB94CD1F767_003","type":"event","event_type":"press","event_id":"A2B861"}
{"id":"7CB94CD1F767_003","type":"event","event_type":"release","event_id":"A2B861"}
```

## 编译

```bash
cd applications/home_assistant/433_gateway
make -j4
# 产物: build_out/433_gateway.bin
```

## 配置

所有配置在 `433_gateway/433_gateway/app_config.h`：

```c
#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "event"
#define DEVICE_NAME        "433网关"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "1.0.1"
#define HA_PUSH_DEFAULT_PORT  9101
```

## 配网

1. 连续开机 3 次进入 BLE 配网模式
2. 通过蓝牙连接发送 WiFi 凭据
3. 设备重启后连接 WiFi
