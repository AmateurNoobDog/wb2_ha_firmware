[English](README_EN.md) | 中文

# example_notify_tts — 语音合成固件

基于 Ai-WB2（BL602）+ 外部 TTS 语音合成模块的固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

**当前版本: 1.1.0**

## 硬件连接

| 通道 | GPIO | 波特率 | 说明 |
|------|------|--------|------|
| UART0 调试 TX | 16 | 115200 | 调试串口 |
| UART0 调试 RX | 7 | 115200 | 调试串口 |
| UART1 TTS TX | 4 | 9600 | TTS 模块发送 |
| UART1 TTS RX | 11 | 9600 | TTS 模块接收 |

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `tts_handler.c/h` | 设备回调：`get_device` 返回实体定义，`set_state` 处理朗读/音量/语速指令 |
| `tw_tts_app.c/h` | TTS 语音合成协议实现（帧格式 `0xFD` + UTF-8） |
| `uart_app.c/h` | UART0/UART1 初始化与收发封装 |
| `app_config.h` | 设备配置（引脚/波特率/参数范围） |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"语音合成","model":"Ai-WB2-TW-TTS","sw_version":"1.1.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"notify","name":"TTS","icon":"mdi:speaker-message"},
   {"id":"ACD8297A605D_002","type":"number","name":"音量","icon":"mdi:volume-high","min":0,"max":9,"step":1},
   {"id":"ACD8297A605D_003","type":"number","name":"语速","icon":"mdi:volume-vibrate","min":0,"max":9,"step":1}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_002","type":"number","value":5},
  {"id":"ACD8297A605D_003","type":"number","value":5}
]}
```

### 控制

```json
{"id":"ACD8297A605D_001","text":"你好世界"}         // TTS 朗读
{"id":"ACD8297A605D_002","value":7}                // 设置音量（0-9）
{"id":"ACD8297A605D_003","value":3}                // 设置语速（0-9）
```

### 推送

音量/语速变更时自动推送到 HA 的 9101 端口：
```json
{"entities":[
  {"id":"ACD8297A605D_002","type":"number","value":7},
  {"id":"ACD8297A605D_003","type":"number","value":3}
]}
```

## 编译

```bash
cd applications/home_assistant/example_notify_tts
make -j4
# 产物: build_out/example_notify_tts.bin
```

## 配置

所有配置在 `example_notify_tts/example_notify_tts/app_config.h`：

```c
#define UART_TTS_TX_PIN     4           // TTS 模块 TX
#define UART_TTS_RX_PIN     11          // TTS 模块 RX
#define UART_TTS_BAUDRATE   9600

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "tts"
#define DEVICE_NAME        "语音合成"
#define DEVICE_MODEL       "Ai-WB2-TW-TTS"
#define DEVICE_SW_VERSION  "1.1.0"

#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
```

## 配网

1. 连续开机 3 次进入 BLE 配网模式
2. 通过蓝牙连接发送 WiFi 凭据
3. 设备重启后连接 WiFi
