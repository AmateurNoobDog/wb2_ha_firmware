# Ai-Thinker WB2 Home Assistant 固件项目集

基于 [Ai-Thinker-WB2 (BL602)](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 的智能家居应用固件。
设备通过 WiFi 接入局域网,使用轻量 TCP JSON 协议与 Home Assistant 集成
(配套集成见 [`ha_ai_thinker_home`](https://gitee.com/amateur-dog/ha_ai_thinker_home))。

**当前版本: 0.9.1**

## 项目结构

```
applications/home_assistant/
├── ha_lib/       # 共享库:设备无关的 TCP JSON 服务器 + JSON 解析器 + 推送模块
├── ha_common/    # 公共组件:store(wifi/easyflash)、wifi_sta、blufi_app(蓝牙配网)、ha_mdns
├── light/        # RGB 彩灯固件(3 路 PWM,设备类型 light)
├── radar_rd_01/  # 雷达存在检测固件(RD-01 芯片,设备类型 radar)
└── switch/       # 3 路智能开关固件(GPIO 继电器,设备类型 switch)
```

### ha_lib(共享组件)
- `ha_device.h`  — 设备抽象:类型、名称、端口、`get_state`/`set_state` 回调
- `ha_json.c/h`  — 极简 JSON 字段解析(`ha_json_str`/`ha_json_int`)
- `tcp_json_server.c` — 通用 TCP 服务:响应 `mac/type/name` + 设备字段,`cmd:set` 分发到设备回调
- `ha_push.c/h`  — 推送模块(TCP 推送至 HA 配置的目标 IP/端口)

新设备只需实现自己的 handler 并注册 `ha_device_t` 即可复用服务器。

### ha_common(公共组件)
- `store.h/c`    — WiFi 配置持久化(EasyFlash),BOOT_CNT 连续重启计数
- `wifi_sta.h/c` — WiFi STA 模式启动,事件驱动
- `blufi_app.h/c`— BLE BluFi 配网,配网成功后自动重启
- `ha_mdns.h/c`  — mDNS 服务注册(`_aitinker._tcp`,端口 9100,TXT: type/name)

## 使用方法

### 1. 克隆到 SDK

基于官方 [Ai-Thinker-WB2 SDK](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2):

```bash
git clone --recursive https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
cd Ai-Thinker-WB2/applications
git clone git@gitee.com:amateur-dog/wb2_ha_firmware.git home_assistant
```

### 2. 编译

需要完整 SDK 环境(BL60X_SDK_PATH 指向 SDK 根目录,含 riscv 工具链):

```bash
cd applications/home_assistant/switch   # 或 light, radar_rd_01
make -j4
# 产物: build_out/switch.bin (或 light.bin, radar_rd_01.bin)
```

### 3. 烧录

直接串口(以 switch 为例):

```bash
cd <SDK>/tools/flash_tool
./bflb_iot_tool-ubuntu --chipname=BL602 --baudrate=921600 --port=/dev/ttyUSB0 \
  --pt=<SDK>/tools/flash_tool/chips/bl602/partition/partition_cfg_2M.toml \
  --dts=<SDK>/tools/flash_tool/chips/bl602/device_tree/04-bl_factory_params_IoTKitA_40M-20220625.dts \
  --firmware=<project>/build_out/switch.bin
```

### 4. 配网

开机配网逻辑:
```
开机 → BOOT_CNT++
  │
  ├─ BOOT_CNT ≥ 3 → 清零 BOOT_CNT → 进入 BLE BluFi 配网(保留已有 WiFi 配置)
  │
  ├─ 有 WiFi 配置 → 连接 WiFi → got_ip → 清零 BOOT_CNT → 正常工作
  │                                   (TCP/mDNS/Push 启动)
  │
  └─ 无 WiFi 配置 → 清零 BOOT_CNT → 进入 BLE BluFi 配网
```

配网成功后设备自动重启,进入正常工作模式。

WiFi 凭据通过 EasyFlash 持久化(`store.c`),可用 CLI 命令 `cfg_clear` 清除。

## 设备协议(TCP 9100)

请求/响应均为一行 JSON。

查询:
```json
{"cmd":"get"}
```

响应示例(开关):
```json
{"mac":"AC:D8:29:7A:60:5D","type":"switch","name":"智能开关","model":"Ai-WB2-12F",
 "count":3,"names":["开关1","开关2","开关3"],
 "on":0,"on1":0,"on2":0}
```

控制(开关,按通道):
```json
{"cmd":"set","on":1}        // 通道 1
{"cmd":"set","on1":0}       // 通道 2
{"cmd":"set","on2":1}       // 通道 3
```

控制(彩灯):
```json
{"cmd":"set","r":255,"g":128,"b":0}
```

响应示例(雷达):
```json
{"mac":"AC:D8:29:7A:60:5D","type":"radar","name":"雷达","model":"RD-01","sw_version":"0.9.1",
 "motion":0,"presence":0,"push":0}
```

控制(雷达):
```json
{"cmd":"calibrate"}       // 标定无人(校准)
{"cmd":"restore"}         // 恢复默认参数
```

### 字段说明

| 字段 | 类型 | 说明 |
|------|------|------|
| `mac` | string | 设备 MAC 地址 |
| `type` | string | 设备类型:`light`(灯)/ `switch`(开关)/ `radar`(雷达),用于 HA 平台选择 |
| `name` | string | 设备名称,由设备上报 |
| `model` | string | 设备型号,由设备上报 |
| `sw_version` | string | 固件版本号,由设备上报 |
| `count` | int | 开关通道数,由设备上报 |
| `names` | array | 各通道名称,由设备上报 |
| `on`/`on1`/`on2` | int | 各通道状态(0/1) |
| `r`/`g`/`b` | int | 彩灯 RGB 值(0-255) |
| `motion` | int | 运动检测状态(0/1),仅 BODYMOTION/BOTH_STATUS 触发 |
| `presence` | int | 存在检测状态(0/1),任何检测结果均触发 |
| `push` | int | 推送上报开关(0/1) |
| `g0`-`g7` | int | 雷达门能量值(需开启 `RADAR_GATE_DATA_ENABLE`) |
| `scnt`/`mcnt` | int | 调试计数器(需开启 `RADAR_DEBUG_COUNTER_ENABLE`) |

> 设备信息(name/model/sw_version/names 等)均由设备上报,Home Assistant 侧在无上报时才回退默认值。

### motion 与 presence 的区别

雷达芯片输出原始状态值(0-3):
- `0` = 无检测
- `1` = BODYMOTION(运动)
- `2` = STATICMotion(静止)
- `3` = BOTH_STATUS(运动+静止)

转换逻辑:
- **motion**: 仅在 `原始值 == 1 或 3` 时为 1(检测到运动)
- **presence**: 在 `原始值 >= 1 且 <= 3` 时为 1(检测到任何存在)

### mDNS 自动发现

设备启动后注册 mDNS 服务:
- 服务类型: `_aitinker._tcp`
- 主机名: `Ai-{type}-{MAC后3字节}.local` (如 `Ai-light-1D94F1.local`)
- TXT 记录: `type=light`, `name=彩灯`

Home Assistant 通过 zeroconf 自动发现局域网内的设备。

## 配置

- **light**:引脚/通道见 `light/light/app_config.h`(`LED_*_PIN`)
- **switch**:继电器引脚、极性、通道名见 `switch/switch/app_config.h`
  (`SWITCH_PINS`、`RELAY_ACTIVE_HIGH`、`SWITCH_NAMES`)
- **radar_rd_01**:雷达配置见 `radar_rd_01/radar_rd_01/app_config.h`
  - 调试开关:`RADAR_GATE_DATA_ENABLE`(门数据)/ `RADAR_DEBUG_COUNTER_ENABLE`(计数器)

## 依赖说明

- 基于官方 [Ai-Thinker-WB2](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 编译;
- Makefile 结构镜像官方 `applications/bluetooth/blufi` 示例(BLE 配网环境一致);
- 本仓库仅包含应用代码,不含 SDK 本体。

## 相关仓库

- 固件仓库:[amateur-dog/wb2_ha_firmware](https://gitee.com/amateur-dog/wb2_ha_firmware)
- Home Assistant 集成:[amateur-dog/ha_ai_thinker_home](https://gitee.com/amateur-dog/ha_ai_thinker_home)
