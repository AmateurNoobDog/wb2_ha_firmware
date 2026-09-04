# light — RGB 彩灯固件

基于 Ai-WB2(BL602) 的 RGB 彩灯固件,支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

## 硬件连接

| 功能 | GPIO | PWM 通道 | 说明 |
|------|------|----------|------|
| LED 红色 | 14 | CH4 | PWM 输出 |
| LED 绿色 | 17 | CH2 | PWM 输出 |
| LED 蓝色 | 3 | CH3 | PWM 输出 |

- PWM 频率:5000Hz
- RGB 范围:0-255

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口,WiFi 事件处理,`ha_device_t` 注册 |
| `led_handler.c/h` | 设备回调: `get_state` 返回 `"model","r","g","b"` |
| `led.c/h` | LED 底层驱动(PWM 初始化/设置/读取) |
| `store.c/h` | WiFi 凭据持久化(EasyFlash) |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `blufi_app.c/h` | BLE BluFi 配网模块 |
| `app_config.h` | 设备配置(引脚/端口/名称) |

## TCP 协议

端口: **9100**

查询:
```json
{"cmd":"get"}
```

响应:
```json
{"mac":"AC:D8:29:7A:60:5D","type":"wb2","name":"彩灯","model":"Ai-WB2-12F",
 "r":255,"g":128,"b":0}
```

控制:
```json
{"cmd":"set","r":255,"g":0,"b":128}
```

支持单独设置:
```json
{"cmd":"set","r":255}       // 仅设置红色
{"cmd":"set","g":128}       // 仅设置绿色
{"cmd":"set","b":0}         // 仅设置蓝色
```

## 编译

```bash
cd applications/home_assistant/light
make -j4
# 产物: build_out/light.bin
```

## 配置

所有配置在 `light/light/app_config.h`:

```c
#define LED_RED_PIN        14
#define LED_GREEN_PIN      17
#define LED_BLUE_PIN       3
#define LED_RED_CH         4
#define LED_GREEN_CH       2
#define LED_BLUE_CH        3
#define LED_PWM_FREQ       5000

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "wb2"
#define DEVICE_NAME        "彩灯"
#define DEVICE_MODEL       "Ai-WB2-12F"
```
