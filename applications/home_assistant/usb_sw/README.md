# usb_sw — 单路 USB 通断器固件

基于 Ai-WB2（BL602）的单路 USB 通断器固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 控制。

**当前版本: 0.10.0**

## 硬件连接

| 通道 | GPIO | 说明 |
|------|------|------|
| USB 通断 | 4 | 继电器控制（低电平开启） |

- 继电器极性：`RELAY_ACTIVE_HIGH=0`（低电平吸合）
- 通道数：`SWITCH_COUNT=1`

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册 |
| `switch_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回开关状态 |
| `relay.c/h` | 继电器底层驱动（GPIO 初始化/设置/读取） |
| `store.c/h` | WiFi 凭据持久化（EasyFlash） |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `blufi_app.c/h` | BLE BluFi 配网模块 |
| `app_config.h` | 设备配置（引脚/端口/名称/极性） |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"7C:B9:4C:D4:5B:0B","name":"USB通断器","model":"Ai-WB2-01S","sw_version":"0.10.0",
 "entities":[
   {"id":"7CB94CD45B0B_001","type":"switch","name":"USB","icon":"mdi:toggle-switch"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"7CB94CD45B0B_001","type":"switch","on":0}
]}
```

### 控制

```json
{"cmd":"set","id":"7CB94CD45B0B_001","on":1}       // 开启
{"cmd":"set","id":"7CB94CD45B0B_001","on":0}       // 关闭
```

### 推送

设备连接 HA 的 9101 端口，推送开关状态：
```json
{"entities":[
  {"id":"7CB94CD45B0B_001","type":"switch","on":1}
]}
```

## 编译

```bash
cd applications/home_assistant/usb_sw
make -j4
# 产物: build_out/usb_sw.bin
```

## 配置

所有配置在 `usb_sw/usb_sw/app_config.h`：

```c
#define SWITCH_PINS        {4}              // 继电器 GPIO
#define SWITCH_COUNT       1                // 通道数
#define RELAY_ACTIVE_HIGH  0                // 低电平吸合
#define SWITCH_NAMES       {"USB"}          // 通道名称

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "switch"
#define DEVICE_NAME        "USB通断器"
#define DEVICE_MODEL       "Ai-WB2-01S"
#define DEVICE_SW_VERSION  "0.10.0"
```

## 配网

1. 连续开机 3 次进入 BLE 配网模式
2. 通过蓝牙连接发送 WiFi 凭据
3. 设备重启后连接 WiFi
