# light — RGB 彩灯固件

基于 Ai-WB2(BL602) 的 RGB 彩灯固件,支持 WiFi + BLE BluFi 配网 + TCP JSON 控制 + 亮度调节。

**当前版本: 0.9.0**

## 硬件连接

| 功能 | GPIO | PWM 通道 | 说明 |
|------|------|----------|------|
| LED 红色 | 14 | CH4 | PWM 输出 |
| LED 绿色 | 17 | CH2 | PWM 输出 |
| LED 蓝色 | 3 | CH3 | PWM 输出 |

- PWM 频率:5000Hz
- RGB 范围:0-255
- 亮度范围:0-255

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口,WiFi 事件处理,`ha_device_t` 注册,ha_push/ha_mdns 初始化 |
| `led_handler.c/h` | 设备回调: `get_state` 返回 `"model","sw_version","r","g","b","brightness","push"` |
| `led.c/h` | LED 底层驱动(PWM 初始化/设置/读取/亮度控制) |
| `app_config.h` | 设备配置(引脚/端口/名称/版本) |

> blufi_app/store/wifi_sta 位于 `ha_common/` 共享模块。

## TCP 协议

端口: **9100**

查询:
```json
{"cmd":"get"}
```

响应:
```json
{"mac":"AC:D8:29:7A:60:5D","type":"light","name":"彩灯","model":"Ai-WB2-12F","sw_version":"0.9.0",
 "r":255,"g":128,"b":0,"brightness":200,"push":0}
```

控制:
```json
{"cmd":"set","r":255,"g":128,"b":0,"brightness":200}
```

支持单独设置:
```json
{"cmd":"set","r":255}           // 仅设置红色
{"cmd":"set","g":128}           // 仅设置绿色
{"cmd":"set","b":0}             // 仅设置蓝色
{"cmd":"set","brightness":128}  // 仅设置亮度
```

推送配置:
```json
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}
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
#define DEVICE_TYPE        "light"
#define DEVICE_NAME        "彩灯"
#define DEVICE_MODEL       "Ai-WB2-12F"
#define DEVICE_SW_VERSION  "0.9.0"
```
