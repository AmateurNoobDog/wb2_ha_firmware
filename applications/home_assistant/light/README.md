# light — RGB 彩灯固件

基于 Ai-WB2（BL602）的 RGB 彩灯固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

**当前版本: 0.10.0**

## 硬件连接

| 功能 | GPIO | PWM 通道 | 说明 |
|------|------|----------|------|
| LED 红色 | 14 | CH4 | PWM 输出 |
| LED 绿色 | 17 | CH2 | PWM 输出 |
| LED 蓝色 | 3 | CH3 | PWM 输出 |

- PWM 频率：5000Hz
- RGB 范围：0-255

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `led_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回 RGB 状态 |
| `led.c/h` | LED 底层驱动（PWM 初始化/设置/读取） |
| `store.c/h` | WiFi 凭据持久化（EasyFlash） |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `blufi_app.c/h` | BLE BluFi 配网模块 |
| `app_config.h` | 设备配置（引脚/端口/名称） |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"彩灯","model":"Ai-WB2-12F","sw_version":"0.10.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"light","name":"彩灯","icon":"mdi:lightbulb"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"light","r":255,"g":128,"b":0,"brightness":255}
]}
```

### 控制

```json
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":128}          // 设置颜色
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":0,"brightness":128}  // 设置颜色和亮度
```

### 推送

设备连接 HA 的 9101 端口，推送灯光状态：
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"light","r":255,"g":128,"b":0,"brightness":255}
]}
```

## 编译

```bash
cd applications/home_assistant/light
make -j4
# 产物: build_out/light.bin
```

## 配置

所有配置在 `light/light/app_config.h`：

```c
#define LED_RED_PIN        14
#define LED_GREEN_PIN      17
#define LED_BLUE_PIN       3
#define LED_RED_CH         4
#define LED_GREEN_CH       2
#define LED_BLUE_CH        3
#define LED_PWM_FREQ       5000

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "light"
#define DEVICE_NAME        "彩灯"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "0.10.0"
```
