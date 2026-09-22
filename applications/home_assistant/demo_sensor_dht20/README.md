[English](README_EN.md) | 中文

# demo_sensor_dht20 — DHT20 温湿度传感器固件

基于 Ai-WB2（BL602）+ DHT20 温湿度传感器的固件，支持 WiFi + BLE BluFi 配网 + TCP JSON 上报。

**当前版本: 1.1.0**

## 硬件连接

| 功能 | GPIO | 说明 |
|------|------|------|
| I2C SCL | 12 | DHT20 时钟线 |
| I2C SDA | 3 | DHT20 数据线 |

- I2C 频率：100kHz
- 供电：3.3V

## 文件说明

| 文件 | 说明 |
|------|------|
| `main.c` | 应用入口，WiFi 事件处理，传感器读取任务，`ha_device_t` 注册 |
| `dht20.c/h` | DHT20 传感器驱动（I2C 通信、CRC8 校验、数据转换） |
| `app_config.h` | 设备配置（I2C 引脚/端口/名称/上报间隔） |

## TCP 协议（v2）

端口：**9100**

### 获取设备信息

```json
{"cmd":"get_device"}
```

响应：
```json
{"mac":"AC:D8:29:7A:60:5D","name":"温湿度传感器","model":"Ai-WB2/DHT20","sw_version":"1.1.0",
 "entities":[
   {"id":"ACD8297A605D_001","type":"sensor","name":"温度","icon":"mdi:thermometer","device_class":"temperature","unit":"°C"},
   {"id":"ACD8297A605D_002","type":"sensor","name":"湿度","icon":"mdi:water-percent","device_class":"humidity","unit":"%"}
 ]}
```

### 获取状态

```json
{"cmd":"get_state"}
```

响应：
```json
{"state":"online","entities":[
  {"id":"ACD8297A605D_001","type":"sensor","value":29.5},
  {"id":"ACD8297A605D_002","type":"sensor","value":58.9}
]}
```

### 主动推送

设备每 5 秒读取一次 DHT20，温度或湿度变化 ≥0.1 时自动推送到 HA 的 9101 端口：
```json
{"entities":[
  {"id":"ACD8297A605D_001","type":"sensor","value":29.5},
  {"id":"ACD8297A605D_002","type":"sensor","value":58.9}
]}
```

## 编译

```bash
cd applications/home_assistant/demo_sensor_dht20
make -j4
# 产物: build_out/demo_sensor_dht20.bin
```

## 配置

所有配置在 `demo_sensor_dht20/demo_sensor_dht20/app_config.h`：

```c
#define DHT20_I2C_SCL_PIN       12
#define DHT20_I2C_SDA_PIN       3
#define DHT20_I2C_FREQ          100000

#define SENSOR_READ_INTERVAL_MS  5000

#define TCP_SERVER_PORT    9100
#define DEVICE_TYPE        "sensor"
#define DEVICE_NAME        "温湿度传感器"
#define DEVICE_MODEL       "Ai-WB2/DHT20"
#define DEVICE_SW_VERSION  "1.1.0"
```

## 配网

1. 连续开机 3 次进入 BLE 配网模式
2. 通过蓝牙连接发送 WiFi 凭据
3. 设备重启后连接 WiFi
