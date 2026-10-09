[English](README_EN.md) | 中文

# example_radar_rd03d — Rd-03D_V2 毫米波雷达多目标固件

基于 Ai-WB2（BL602）+ 安信可 Rd-03D_V2 24GHz 毫米波雷达的固件，支持 WiFi + BLE BluFi
配网 + TCP JSON 上报，可将雷达的多目标坐标/速度接入 Home Assistant。

**当前版本: 1.2.0**

## 硬件连接

| 功能 | Ai-WB2 引脚 | Rd-03D_V2 引脚 | 参数 |
|------|------------|----------------|------|
| 雷达串口 | GPIO4 (TX) / GPIO5 (RX) | RXD / TXD | UART1，256000 8N1 |
| 日志 console | GPIO16 (TX) / GPIO7 (RX) | 接 USB 串口工具 | UART0，115200 8N1 |

> **注意**：`printf` 日志硬绑定在 **UART0**（GPIO16/GPIO7@115200，系统启动时绑定）。
> 雷达必须使用 **UART1 + GPIO4/GPIO5**，不要接到 UART0，否则日志会以雷达波特率灌入雷达 RX。
>
> 本工程日志走 `uart_app.c` 直写 UART0（`uart_logf()` 格式化输出，`ha_device_t.log` 也挂到 `uart_log`，
> 故 TCP 服务器日志同样从 console 口输出），不经过 `blog`（`LOG_ENABLED_COMPONENTS` 仅保留 `ha_lib` 兜底）。

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi/配网事件处理，雷达接收任务，推送节流，`ha_device_t` 注册 |
| `uart_app.c/h` | UART 薄封装（移植自 `example_notify_tts`）：`uart_init()` 统一初始化 uart0 日志口 + uart1 雷达口，`uart_log/uart_logf` 日志直写 |
| `rd03d.c/h` | Rd-03D_V2 串口驱动（逐字节状态机帧解析、配置命令，协议见 [通信.md](./通信.md)） |
| `rd03d_handler.c/h` | HA 设备回调：`get_device` 实体定义、`get_state` 状态、`push_cfg` 命令、帧缓存 |
| `app_config.h` | 设备配置（串口/节流/端口/名称） |

## 实体列表

实体 ID 格式：`{MAC 12位hex}_{3位序号}`，共 **10 个**：

| 序号 | 类型 | 名称 | 单位 |
|------|------|------|------|
| 001 | `binary_sensor` | 有人 | — |
| 002 / 003 / 004 | `sensor` | 目标1 X / Y / 速度 | mm / mm / cm/s |
| 005 / 006 / 007 | `sensor` | 目标2 X / Y / 速度 | mm / mm / cm/s |
| 008 / 009 / 010 | `sensor` | 目标3 X / Y / 速度 | mm / mm / cm/s |

速度负值表示目标正在靠近雷达。目标不存在时 `value` 为 `null`。

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"毫米波雷达","model":"Rd-03D_V2",
 "manufacturer":"Ai-Thinker","sw_version":"1.2.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"binary_sensor","name":"有人","icon":"mdi:motion-sensor"},
   {"id":"ACD8297A605D_002","type":"sensor","name":"目标1 X","icon":"mdi:axis-x-light","unit":"mm"},
   {"id":"ACD8297A605D_003","type":"sensor","name":"目标1 Y","icon":"mdi:axis-y-light","unit":"mm"},
   {"id":"ACD8297A605D_004","type":"sensor","name":"目标1 速度","icon":"mdi:speedometer","unit":"cm/s"}
 ],
 "offline_timeout":0}
```

`offline_timeout` 固定为 **0**：本设备**不启用推送-only 模式**，HA 始终保持轮询——
因为目标消失时设备不推送，只有轮询才能把"目标离开"反映到 HA。

### 获取状态

```json
{"cmd":"get_state"}
```

响应（前方有 2 个目标，第 3 槽位空）：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1},
  {"id":"ACD8297A605D_002","type":"sensor","value":159},
  {"id":"ACD8297A605D_003","type":"sensor","value":286},
  {"id":"ACD8297A605D_004","type":"sensor","value":0},
  {"id":"ACD8297A605D_005","type":"sensor","value":561},
  {"id":"ACD8297A605D_006","type":"sensor","value":4503},
  {"id":"ACD8297A605D_007","type":"sensor","value":-16},
  {"id":"ACD8297A605D_008","type":"sensor","value":null},
  {"id":"ACD8297A605D_009","type":"sensor","value":null},
  {"id":"ACD8297A605D_010","type":"sensor","value":null}
]}
```

### 控制命令

```json
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}   // 配置推送目标
```

## 主动推送（端口 9101）

推送目标通过 `push_cfg` 命令配置后生效。推送规则：

- **有人状态变化**：立即推送单实体
  ```json
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1}
  ```
- **各目标 X/Y/速度**：每目标独立一条消息，按速度自适应节流
  ```c
  interval = clamp(PUSH_MAX_MS / (1 + |speed|), PUSH_MIN_MS, PUSH_MAX_MS)
  ```
  默认 `PUSH_MIN_MS=250`、`PUSH_MAX_MS=5000`：目标移动越快推送越频繁（最快 250ms），
  静止目标最慢 5s 推一次；数值未变化不推送。
  ```json
  {"entities":[
    {"id":"ACD8297A605D_002","type":"sensor","value":159},
    {"id":"ACD8297A605D_003","type":"sensor","value":286},
    {"id":"ACD8297A605D_004","type":"sensor","value":0}
  ]}
  ```
- **目标消失不推送**，由 HA 轮询 `get_state` 更新；目标再次出现按"首次出现"立即推送。

## 编译

```bash
cd applications/home_assistant/example_radar_rd03d
make -j4
# 产物: build_out/example_radar_rd03d.bin
```

## 配置

所有配置在 `example_radar_rd03d/example_radar_rd03d/app_config.h`：

```c
#define RD03D_UART_ID        1        // 雷达接 UART1
#define RD03D_UART_TX_PIN    4        // WB2 TX -> 雷达 RX
#define RD03D_UART_RX_PIN    5        // WB2 RX <- 雷达 TX
#define RD03D_UART_BAUDRATE  256000   // 雷达默认波特率
#define RD03D_POWERON_DELAY_MS 2000   // 雷达上电等待

#define PUSH_MAX_MS  5000             // 最慢推送周期
#define PUSH_MIN_MS  250              // 最快推送周期
#define RADAR_LOG_PERIOD_MS 0         // 解析结果打印周期(ms), 0=关闭

#define TCP_SERVER_PORT  9100
#define DEVICE_NAME      "毫米波雷达"
#define DEVICE_MODEL     "Rd-03D_V2"
```

## 配网

1. 连续开机 3 次进入 BLE BluFi 配网模式
2. 通过蓝牙发送 WiFi 凭据
3. 设备重启后连接 WiFi，got_ip 后启动 TCP 服务器与 mDNS 注册

WiFi 凭据可用 CLI 命令 `cfg_clear` 清除。
