# radar_rd_01 — 雷达存在检测固件

基于 Ai-WB2(BL602) + Ai-Thinker RD-01 雷达芯片的人体存在检测固件。
BL602 作为 SPI 从机接收雷达数据,通过 WiFi + TCP JSON 上报运动/存在状态到 Home Assistant。

**当前版本: 0.8.0**

## 硬件连接

### RD-01 雷达芯片接口

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

1. BL602 通过 GPIO 20 给雷达芯片上电
2. 雷达芯片作为 SPI 主机,以 ~12.5MHz 发送检测数据
3. BL602 SPI 从机接收数据,放入队列(`FUNC_QUEUE_SIZE=34`)
4. 数据处理任务解析雷达帧,提取运动状态
5. TCP 服务器每 1s 轮询上报状态到 HA

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口,WiFi 事件处理,`ha_device_t` 注册,雷达数据回调 |
| `radar_handler.c/h` | 设备回调: `get_state` 返回 `"model","sw_version","motion","presence","push"` |
| `store.c/h` | WiFi 凭据 + 推送配置持久化(EasyFlash) |
| `wifi_sta.c/h` | WiFi STA 连接管理 |
| `blufi_app.c/h` | BLE BluFi 配网模块(支持自定义数据接收 HA IP) |
| `app_config.h` | 设备配置(引脚/端口/名称/调试开关) |
| `D103/` | 雷达芯片驱动库(原始,未修改) |
| `bodysense_lib/` | 人体感应算法库 |
| `axk_factory/` | 出厂测试库 |
| `hal_wifi/` | WiFi HAL 层 |
| `axk_cfg/` | 配置管理库 |

## TCP 协议

端口: **9100**

查询:
```json
{"cmd":"get"}
```

响应(默认模式):
```json
{"mac":"AC:D8:29:7A:60:5D","type":"radar","name":"雷达","model":"RD-01","sw_version":"0.8.0",
 "motion":0,"presence":0,"push":0}
```

控制:
```json
{"cmd":"calibrate"}       // 标定无人(校准阈值)
{"cmd":"restore"}         // 恢复默认参数
{"cmd":"push_cfg","ip":"192.168.1.100","port":9101}  // 配置推送目标
```

### motion 与 presence 的区别

雷达芯片输出原始状态值(0-3):
| 原始值 | 含义 | motion | presence |
|--------|------|--------|----------|
| 0 | 无检测 | 0 | 0 |
| 1 | BODYMOTION(运动) | 1 | 1 |
| 2 | STATICMotion(静止) | 0 | 1 |
| 3 | BOTH_STATUS(运动+静止) | 1 | 1 |

- **motion**: 仅在检测到运动时为 1(BODYMOTION 或 BOTH_STATUS)
- **presence**: 任何检测结果均视为存在(原始值 1/2/3)

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
// 雷达硬件
#define RADAR_CS_PIN          22      // SPI 片选
#define RADAR_POWER_PIN       25      // 电源控制(实际使用 20)

// 设备身份
#define TCP_SERVER_PORT       9100
#define DEVICE_TYPE           "radar"
#define DEVICE_NAME           "雷达"
#define DEVICE_MODEL          "RD-01"
#define DEVICE_SW_VERSION     "0.8.0"

// 推送配置
#define HA_PUSH_DEFAULT_PORT  9101    // HA 推送监听端口

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

- SPI 从机时钟受限于 BL602 SPI 外设,最高 ~12.5MHz
- `spi_timeout_handle` 未创建(始终为 NULL),SPI 超时回调未启用
- `timer_60ms_handle` 为一次性定时器(period 60ms),用于雷达数据处理调度
- 推送功能已实现但回退为仅 1s 轮询模式
