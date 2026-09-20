# Ai-Thinker WB2 Home Assistant 固件项目集

基于 [Ai-Thinker-WB2 (BL602)](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 的智能家居应用固件。
设备通过 WiFi 接入局域网，使用轻量 TCP JSON 协议与 Home Assistant 集成
（配套集成见 [`and_home`](https://gitee.com/AmateurNoobDog/and_home)）。

**当前版本: 1.0.1**

## 项目结构

```
applications/home_assistant/
├── ha_lib/         # 共享库：设备无关的 TCP JSON 服务器 + cJSON 解析器 + 推送模块 + 域名解析
├── ha_common/      # 公共组件：store(wifi/easyflash)、wifi_sta、blufi_app(蓝牙配网)、ha_mdns
├── light/          # RGB 彩灯固件（3 路 PWM，设备类型 light）
├── radar_rd_01/    # 雷达存在检测固件（RD-01 芯片，设备类型 radar）
├── switch/         # 3 路智能开关固件（GPIO 继电器，设备类型 switch）
├── usb_sw/         # 单路 USB 通断器固件（IO4 低电平开启，设备类型 switch）
├── 433_gateway/    # 433 遥控网关固件（UART1 接收，设备类型 event）
└── tts/            # TTS 语音合成固件（TW-TTS 模块，设备类型 tts）
```

### ha_lib（共享组件）
- `ha_device.h`  — 设备抽象：类型、名称、型号、厂商、版本、端口、`get_device`/`get_state`/`set_state` 回调
- `ha_json.c/h`  — 基于 cJSON 的 JSON 字段解析（`ha_json_str`/`ha_json_int`）
- `tcp_json_server.c` — 通用 TCP 服务：响应 `get_device`/`get_state`/`set` 命令
- `ha_push.c/h`  — 推送模块（TCP 推送至 HA 配置的目标主机/端口，支持 IP 和域名）
- `ha_mdns_query.c/h` — 域名解析（.local 走 mDNS，其余走 DNS，带缓存）

新设备只需实现自己的 handler 并注册 `ha_device_t` 即可复用服务器。

### ha_common（公共组件）
- `store.h/c`    — WiFi 配置持久化（EasyFlash），BOOT_CNT 连续重启计数
- `wifi_sta.h/c` — WiFi STA 模式启动，事件驱动
- `blufi_app.h/c`— BLE BluFi 配网，配网成功后自动重启
- `ha_mdns.h/c`  — mDNS 服务注册（`_and._tcp`，端口 9100，TXT: type/name）

## 使用方法

### 1. 克隆到 SDK

基于官方 [Ai-Thinker-WB2 SDK](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2)：

```bash
git clone --recursive https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
cd Ai-Thinker-WB2/applications
git clone https://gitee.com/AmateurNoobDog/wb2_ha_firmware.git home_assistant
```

### 2. 编译

需要完整 SDK 环境（BL60X_SDK_PATH 指向 SDK 根目录，含 riscv 工具链）：

```bash
cd applications/home_assistant/switch   # 或 light, radar_rd_01, 433_gateway, usb_sw, tts
make -j4
# 产物: build_out/switch.bin
```

### 3. 烧录

直接串口（以 switch 为例）：

```bash
cd <SDK>/tools/flash_tool
./bflb_iot_tool-ubuntu --chipname=BL602 --baudrate=921600 --port=/dev/ttyUSB0 \
  --pt=<project>/img_conf/partition_cfg_2M.toml \
  --dts=<project>/img_conf/bl_factory_params_IoTKitA_40M.dts \
  --firmware=<project>/build_out/switch.bin
```

### 4. 配网

开机配网逻辑：
```
开机 → BOOT_CNT++
  │
  ├─ BOOT_CNT ≥ 3 → 清零 BOOT_CNT → 进入 BLE BluFi 配网（保留已有 WiFi 配置）
  │
  ├─ 有 WiFi 配置 → 连接 WiFi → got_ip → 清零 BOOT_CNT → 正常工作
  │                                   （TCP/mDNS/Push 启动）
  │
  └─ 无 WiFi 配置 → 清零 BOOT_CNT → 进入 BLE BluFi 配网
```

配网成功后设备自动重启，进入正常工作模式。

WiFi 凭据通过 EasyFlash 持久化（`store.c`），可用 CLI 命令 `cfg_clear` 清除。

## 设备协议（TCP 9100）

协议版本：**v2（实体驱动架构）**

请求/响应均为一行 JSON，以 `\n` 分隔。

### 获取设备信息和实体定义

```json
{"cmd":"get_device"}
```

响应示例（开关）：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"智能开关","model":"Ai-WB2-12F","manufacturer":"AND-DIY","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"switch","name":"开关1","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_002","type":"switch","name":"开关2","icon":"mdi:toggle-switch"},
   {"id":"ACD8297A605D_003","type":"switch","name":"开关3","icon":"mdi:toggle-switch"}
 ]}
```

响应示例（433 网关）：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"433网关","model":"Ai-WB2-12F","manufacturer":"AND-DIY","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"button","name":"配对","icon":"mdi:remote","action":"pair"},
   {"id":"ACD8297A605D_002","type":"button","name":"重置","icon":"mdi:restore","action":"reset"},
   {"id":"ACD8297A605D_003","type":"event","name":"键值","icon":"mdi:remote"}
 ]}
```

### 获取实体状态

```json
{"cmd":"get_state"}
```

响应示例：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":0},
  {"id":"ACD8297A605D_002","type":"switch","on":1},
  {"id":"ACD8297A605D_003","type":"switch","on":0}
]}
```

### 控制实体

```json
{"cmd":"set","id":"ACD8297A605D_002","on":1}       // 控制开关 2
{"cmd":"set","id":"ACD8297A605D_001","r":255,"g":0,"b":128}  // 控制灯光
```

### 通用命令

```json
{"cmd":"pair"}            // 配对模式（433 网关）
{"cmd":"reset"}           // 重置配对
{"cmd":"calibrate"}       // 标定无人（雷达）
{"cmd":"restore"}         // 恢复默认参数（雷达）
```

### 设备主动推送（端口 9101）

设备连接 HA 的 9101 端口，推送实体状态：

单实体格式（433/key_sensor）：
```json
{"id":"ACD8297A605D_003","type":"event","event_type":"press","event_id":"A2B861"}
```

多实体格式（switch）：
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"switch","on":1},
  {"id":"ACD8297A605D_002","type":"switch","on":0}
]}
```

### 实体类型矩阵

| 类型 | 有 command | 有 state | 说明 |
|------|-----------|----------|------|
| `light` | set（r/g/b/brightness） | 有 | RGB 灯 |
| `switch` | set（on） | 有 | 继电器开关 |
| `binary_sensor` | — | 有 | 二进制传感器（雷达 motion/presence） |
| `button` | action cmd | — | 按钮（配对/重置） |
| `event` | — | 有 | 事件（433 按键按下/释放） |
| `sensor` | — | 有 | 数据传感器（键值） |
| `notify` | set（text） | — | 通知（TTS 语音合成） |
| `number` | set（value） | 有 | 数值（TTS 音量/语速） |

### 实体 ID 格式

```
{MAC完整hex（12位）}_{3位流水号}
```

示例：`ACD8297A605D_001`、`ACD8297A605D_002`

### mDNS 自动发现

设备启动后注册 mDNS 服务：
- 服务类型：`_and._tcp`
- 主机名：`and-{type}-{MAC后3字节}.local`（如 `and-light-1D94F1.local`）

Home Assistant 通过 zeroconf 自动发现局域网内的设备。

## 配置

- **light**: 引脚/通道见 `light/light/app_config.h`（`LED_*_PIN`）
- **switch**: 继电器引脚、极性、通道名见 `switch/switch/app_config.h`
  （`SWITCH_PINS`、`RELAY_ACTIVE_HIGH`、`SWITCH_NAMES`）
- **usb_sw**: 单路 USB 通断器引脚见 `usb_sw/usb_sw/app_config.h`
  （`SWITCH_PINS`=IO4、`RELAY_ACTIVE_HIGH`=0 低电平开启）
- **433_gateway**: 433 遥控接收器配置见 `433_gateway/433_gateway/app_config.h`
  （UART1: TX=GPIO6, RX=GPIO4, 9600bps; 配对按钮: GPIO5）
- **tts**: TTS 语音合成配置见 `tts/tts/app_config.h`
  （UART1: TX=GPIO4, RX=GPIO11, 9600bps 连接 TW-TTS 模块）
- **radar_rd_01**: 雷达配置见 `radar_rd_01/radar_rd_01/app_config.h`
  - 调试开关：`RADAR_GATE_DATA_ENABLE`（门数据）/ `RADAR_DEBUG_COUNTER_ENABLE`（计数器）

## 依赖说明

- 基于官方 [Ai-Thinker-WB2](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 编译；
- Makefile 结构镜像官方 `applications/bluetooth/blufi` 示例（BLE 配网环境一致）；
- 本仓库仅包含应用代码，不含 SDK 本体。

## 相关仓库

- 固件仓库：[amateur-dog/wb2_ha_firmware](https://gitee.com/amateur-dog/wb2_ha_firmware)
- Home Assistant 集成：[AmateurNoobDog/and_home](https://gitee.com/AmateurNoobDog/and_home)
