# tts — TTS 语音合成固件

基于 Ai-WB2（BL602）+ TW-TTS 语音合成模块的固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

**当前版本: 1.0.1**

## 硬件连接

| 功能 | GPIO | 波特率 | 说明 |
|------|------|--------|------|
| UART0 TX（调试） | 16 | 115200 | 调试串口输出 |
| UART0 RX（调试） | 7 | 115200 | 调试串口输入 |
| UART1 TX（TTS） | 4 | 9600 | 连接 TW-TTS 模块 |
| UART1 RX（TTS） | 11 | 9600 | 连接 TW-TTS 模块 |

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `tts_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回音量/语速状态 |
| `tw_tts_app.c/h` | TW-TTS 语音合成驱动（合成/设置音量/设置语速） |
| `uart_app.c/h` | UART 驱动（双串口初始化/收发/打印） |
| `app_config.h` | 设备配置（引脚/端口/名称/实体定义） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"语音合成","model":"Ai-WB2-TW-TTS","manufacturer":"AND-DIY","sw_version":"1.0.1",
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

语音合成（notify 实体）：
```json
{"cmd":"set","id":"ACD8297A605D_001","text":"你好世界"}
```

设置音量（number 实体，范围 0-9）：
```json
{"cmd":"set","id":"ACD8297A605D_002","value":7}
```

设置语速（number 实体，范围 0-9）：
```json
{"cmd":"set","id":"ACD8297A605D_003","value":5}
```

音量和语速修改后自动保存到 EasyFlash，重启后恢复。

### 推送

设备连接 HA 的 9101 端口，推送音量/语速状态：
```json
{"entities":[
  {"id":"ACD8297A605D_002","type":"number","value":7},
  {"id":"ACD8297A605D_003","type":"number","value":5}
]}
```

## TTS 芯片通信协议

TW-TTS 模块通过 UART1 以 9600bps 通信，帧格式：

| 字节 | 值 | 说明 |
|------|-----|------|
| 0 | `0xFD` | 帧头 |
| 1-2 | 长度 | 数据长度 = 文本字节数 + 2（大端） |
| 3 | `0x01` | 命令字：语音合成 |
| 4 | `0x04` | 编码格式：GB2312/UTF-8 |
| 5+ | 文本数据 | 要合成的文本 |

## 编译

```bash
cd applications/home_assistant/tts
make -j4
# 产物: build_out/tts.bin
```

## 配置

所有配置在 `tts/tts/app_config.h`：

```c
// UART 引脚
#define UART_DEBUG_TX_PIN   16
#define UART_DEBUG_RX_PIN   7
#define UART_TTS_TX_PIN     4
#define UART_TTS_RX_PIN     11

// 设备信息
#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "tts"
#define DEVICE_NAME        "语音合成"
#define DEVICE_MODEL       "Ai-WB2-TW-TTS"
#define DEVICE_MANUFACTURER "AND-DIY"
#define DEVICE_SW_VERSION  "1.0.1"

// TTS 参数范围
#define TTS_VOLUME_MIN      0
#define TTS_VOLUME_MAX      9
#define TTS_SPEED_MIN       0
#define TTS_SPEED_MAX       9
```

## 实体类型说明

本模块引入了两个新的实体类型：

- **`notify`**：通知实体，接收 `text` 字段触发语音合成播放
- **`number`**：数值实体，接收 `value` 字段设置参数（音量/语速），带 min/max/step 属性
