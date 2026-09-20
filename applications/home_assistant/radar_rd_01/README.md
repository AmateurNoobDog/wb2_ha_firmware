# radar_rd_01 — 雷达存在检测固件

> [English](README_en.md) | **中文**

基于 Ai-Thinker RD-01 模组(BL602 + 雷达芯片一体)的人体存在检测固件。
固件运行在模组内的 BL602 上,通过 SPI/I2C/UART 与模组内的雷达芯片通信,
通过 WiFi + TCP JSON 上报运动/存在状态到 Home Assistant。

**当前版本: 1.0.1**

## 硬件连接

### RD-01 模组内部接口

以下为 BL602 与模组内雷达芯片的内部连接:

| 功能 | BL602 GPIO | 说明 |
|------|------------|------|
| SPI CLK | 11 | SPI 时钟(从机,~12.5MHz) |
| SPI MOSI | 0 | 主出从入(雷达→BL602) |
| SPI CS | 22 | 片选(低电平有效) |
| I2C SCL | 14 | 寄存器配置 |
| I2C SDA | 3 | 寄存器配置 |
| UART1 TX | 16 | 命令通道(256000 baud) |
| UART1 RX | 7 | 命令通道 |
| 电源控制 | 20 | PMOS 开关(高电平断电) |
| EN | 12 | 雷达使能 |
| REXT | 21 | 外部参考电阻 |

### 工作流程

1. BL602 通过 GPIO 20 控制模组内雷达芯片的电源(PMOS 开关)
2. 雷达芯片上电后作为 SPI 主机,以 ~12.5MHz 发送检测数据
3. BL602 SPI 从机接收数据,放入队列(`FUNC_QUEUE_SIZE=34`)
4. 数据处理任务解析雷达帧,提取运动状态
5. TCP 服务器每 1s 轮询上报状态到 HA

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，`ha_device_t` 注册，雷达数据回调 |
| `radar_handler.c/h` | 设备回调：`get_device` 返回实体定义，`get_state` 返回运动/存在状态 |
| `app_config.h` | 设备配置（引脚/端口/名称/调试开关） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |
| `D103/` | RD-01 模组内雷达芯片驱动库（原始，未修改） |
| `bodysense_lib/` | 人体感应算法库 |
| `axk_factory/` | 出厂测试库 |
| `hal_wifi/` | WiFi HAL 层 |
| `axk_cfg/` | 配置管理库 |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"雷达","model":"RD-01","manufacturer":"AND-DIY","sw_version":"1.0.1",
 "entities":[
   {"id":"ACD8297A605D_001","type":"binary_sensor","name":"有人","icon":"mdi:motion-sensor"},
   {"id":"ACD8297A605D_002","type":"binary_sensor","name":"运动","icon":"mdi:run-fast"},
   {"id":"ACD8297A605D_003","type":"button","name":"标定无人","icon":"mdi:cog-counterclockwise","action":"calibrate"},
   {"id":"ACD8297A605D_004","type":"button","name":"恢复默认参数","icon":"mdi:restore","action":"restore"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":0},
  {"id":"ACD8297A605D_002","type":"binary_sensor","value":1}
]}
```

### 控制命令

```json
{"cmd":"calibrate"}       // 标定无人（校准阈值）
{"cmd":"restore"}         // 恢复默认参数
{"cmd":"push_cfg","host":"192.168.1.100","port":9101}  // 配置推送目标（支持 IP 和域名）
```

### 推送

设备主动推送运动/存在状态到 HA 的 9101 端口：
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"binary_sensor","value":1},
  {"id":"ACD8297A605D_002","type":"binary_sensor","value":1}
]}
```

### motion 与 presence 的区别

雷达芯片输出原始状态值(0-3):
| 原始值 | 含义 | motion | presence |
|--------|------|--------|----------|
| 0 | 无检测 | 0 | 0 |
| 1 | BODYMOTION(运动) | 1 | 1 |
| 2 | STATICMotion(静止) | 0 | 1 |
| 3 | BOTH_STATUS(运动+静止) | 1 | 1 |

- **有人（presence）**: 任何检测结果均视为存在（原始值 1/2/3），直接使用原始值
- **运动（motion）**: 带防抖逻辑 — 当雷达报告静止(原始值=2)时，需连续检测 `STATIONARY_CONFIRM_COUNT`（默认10次）才判定为无运动；在此之前仍保持 `motion=1`

防抖机制：每次雷达报告运动状态时调用 `radar_handler_set_motion()`，若原始值为2则递增静止计数器，
达到阈值后 `radar_handler_get_motion()` 才返回0；任何非静止状态会重置计数器。

### 调试模式

在 `app_config.h` 中开启调试开关后,响应会包含额外字段:

`RADAR_GATE_DATA_ENABLE=1` 时:
```json
{"mac":"...","type":"radar","model":"RD-01",
 "motion":0,"on":0,
 "g0":12,"g1":34,"g2":56,"g3":78,"g4":90,"g5":12,"g6":34,"g7":56,
 "scnt":100,"mcnt":5}
```

| 字段 | 说明 |
|------|------|
| `g0`-`g7` | 8 个门的能量值(每门 75cm,共 6m) |
| `scnt` | `get_state` 调用计数器 |
| `mcnt` | 运动事件计数器 |

`RADAR_DEBUG_COUNTER_ENABLE=1` 时(不含门数据):
```json
{"mac":"...","type":"radar","model":"RD-01",
 "motion":0,"on":0,"scnt":100,"mcnt":5}
```

## 编译

```bash
cd applications/home_assistant/radar_rd_01
make -j6
# 产物: build_out/radar_rd_01.bin (~589K)
```

## 配置

所有配置在 `radar_rd_01/radar_rd_01/app_config.h`:

```c
// RD-01 模组硬件(内部接口)
#define RADAR_CS_PIN          22      // SPI 片选(连接模组内雷达芯片)
#define RADAR_POWER_PIN       25      // 电源控制(控制模组内雷达芯片)

// 设备身份
#define TCP_SERVER_PORT       9100
#define TCP_SERVER_STACK      4096
#define DEVICE_TYPE           "radar"
#define DEVICE_NAME           "雷达"
#define DEVICE_MODEL          "RD-01"
#define DEVICE_MANUFACTURER   "AND-DIY"
#define DEVICE_SW_VERSION     "1.0.1"

// 推送配置
#define HA_PUSH_DEFAULT_PORT  9101    // HA 推送监听端口

// 静止判定防抖：连续检测到几次无运动才判定为静止
#define STATIONARY_CONFIRM_COUNT   10

// 调试开关
#define RADAR_GATE_DATA_ENABLE    0   // 门数据上报(0=关闭)
#define RADAR_DEBUG_COUNTER_ENABLE 0  // 调试计数器(0=关闭)
```

## BluFi 自定义数据

雷达固件支持通过 BluFi 接收 HA 推送目标配置,格式:
```json
{"ha_ip":"192.168.1.100","ha_port":9101}
```

## 已知限制

- RD-01 模组内 SPI 从机时钟受限于 BL602 SPI 外设,最高 ~12.5MHz
- `spi_timeout_handle` 未创建(始终为 NULL),SPI 超时回调未启用
- `timer_60ms_handle` 为一次性定时器(period 60ms),用于雷达数据处理调度
- 推送功能已实现但回退为仅 1s 轮询模式
