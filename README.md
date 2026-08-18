# Ai-Thinker WB2 Home Assistant 固件项目集

基于 [Ai-Thinker-WB2 (BL602)](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 的智能家居应用固件。
设备通过 WiFi 接入局域网,使用轻量 TCP JSON 协议与 Home Assistant 集成
(配套集成见 [`ha_ai_thinker_home`](https://gitee.com/amateur-dog/ha_ai_thinker_home))。

## 项目结构

```
applications/home_assistant/
├── ha_lib/     # 共享库:设备无关的 TCP JSON 服务器 + JSON 解析器
├── light/      # RGB 彩灯固件(3 路 PWM,设备类型 wb2)
└── switch/     # 3 路智能开关固件(GPIO 继电器,设备类型 sw)
```

### ha_lib(共享组件)
- `ha_device.h`  — 设备抽象:类型、名称、端口、`get_state`/`set_state` 回调
- `ha_json.c/h`  — 极简 JSON 字段解析(`ha_json_str`/`ha_json_int`)
- `tcp_json_server.c` — 通用 TCP 服务:响应 `mac/type/name` + 设备字段,`cmd:set` 分发到设备回调

新设备只需实现自己的 handler 并注册 `ha_device_t` 即可复用服务器。

## 使用方法

### 1. 获取 SDK 并放置代码

基于官方 [Ai-Thinker-WB2 SDK](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2):

```bash
git clone --recursive https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2.git
cd Ai-Thinker-WB2
# 将本仓库的 applications/home_assistant 复制到 SDK 的 applications/ 下
cp -r <本仓库路径>/applications/home_assistant applications/
```

### 2. 编译

需要完整 SDK 环境(BL60X_SDK_PATH 指向 SDK 根目录,含 riscv 工具链):

```bash
cd applications/home_assistant/switch   # 或 light
make -j4
# 产物: build_out/switch.bin (或 light.bin)
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

- 开机若检测到已保存的 WiFi 配置则直连;
- 否则进入 BLE blufi 配网(使用官方 blufi 手机端/小程序);
- WiFi 凭据通过 EasyFlash 持久化(`store.c`),可用 CLI 命令 `cfg_clear` 清除。

## 设备协议(TCP 9100)

请求/响应均为一行 JSON。

查询:
```json
{"cmd":"get"}
```

响应示例(开关):
```json
{"mac":"AC:D8:29:7A:60:5D","type":"sw","name":"智能开关","model":"WB2",
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

### 字段说明
| 字段 | 说明 |
|------|------|
| `mac` | 设备 MAC |
| `type` | 设备类型:`wb2`(灯)/ `sw`(开关),用于 HA 平台选择 |
| `name` | 设备名称,由设备上报 |
| `model` | 设备型号,由设备上报 |
| `count`/`names` | 开关通道数 / 各通道名称,由设备上报 |
| `on`/`on1`/`on2` | 各通道状态(0/1) |
| `r`/`g`/`b` | 彩灯 RGB |

> 设备信息(name/model/names 等)均由设备上报,Home Assistant 侧在无上报时才回退默认值。

## 配置

- **light**:引脚/通道见 `light/light/app_config.h`(`LED_*_PIN`)
- **switch**:继电器引脚、极性、通道名见 `switch/switch/app_config.h`
  (`SWITCH_PINS`、`RELAY_ACTIVE_HIGH`、`SWITCH_NAMES`)

## 依赖说明

- 基于官方 [Ai-Thinker-WB2](https://gitee.com/Ai-Thinker-Open/Ai-Thinker-WB2) SDK 编译;
- Makefile 结构镜像官方 `applications/bluetooth/blufi` 示例(BLE 配网环境一致);
- 本仓库仅包含应用代码,不含 SDK 本体。

## 相关仓库

- Home Assistant 集成:[amateur-dog/ha_ai_thinker_home](https://gitee.com/amateur-dog/ha_ai_thinker_home)