# mqtt_ctl.py — 控制板 MQTT 控制工具

向控制板(ctrl_reboot)发布控制命令并接收回执的命令行脚本。

## 简介

脚本通过 MQTT 向控制板发布命令,控制被控开发板的启动行为:

- `reboot`:重启被控板(正常启动)
- `flash`:让被控板进入下载(烧录)模式
- `boot`:启动被控板(正常启动)

控制板执行完成后,通过 `wb2/status` 返回回执。

## 环境要求

- Python 3
- `paho-mqtt` 库:`pip install paho-mqtt`
- 运行环境(本机)与 MQTT broker 网络可达

## 用法

```bash
# 发送单条命令并等待回执
python3 mqtt_ctl.py reboot
python3 mqtt_ctl.py flash
python3 mqtt_ctl.py boot

# 指定 broker / 端口(默认 192.168.31.121:1883)
python3 mqtt_ctl.py --broker 192.168.31.121 --port 1883 flash

# 仅监听回执,不发送命令(观察模式)
python3 mqtt_ctl.py --watch
```

### 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `cmd` | - | 命令:`reboot` / `flash` / `boot` |
| `--broker` | `192.168.31.121` | MQTT broker 地址 |
| `--port` | `1883` | MQTT 端口 |
| `--watch` | 关 | 只监听 `wb2/status`,不发送 |
| `--wait` | `6.0` | 等待回执的秒数 |

## 通信协议

| 项 | 值 |
|----|-----|
| 发送 topic | `wb2/control` |
| 回执 topic | `wb2/status` |
| QoS | 发送 1,订阅 1 |

### 命令与回执

| 发送命令 | 回执 |
|----------|------|
| `reboot` | `OK:reboot` |
| `flash` | `OK:flash` |
| `boot` | `OK:boot` |
| 其它 | `ERROR:unknown` |

回执只表示控制板**已执行 GPIO 动作**,不代表被控板结果。

## 控制板 GPIO 行为(固件侧,供排查)

控制板 GPIO 定义:

- `IO14`(GPIO14)= 被控板 **EN**(复位线)
- `IO3`(GPIO3)= 被控板 **BOOT**(启动模式选择)

空闲态:`EN`=输出高(释放复位)、`BOOT`=输出低。引脚始终为输出模式,直到下一条命令。

各命令的时序:

- **reboot / boot**:`BOOT`=低(100ms)→ `EN` 拉低 200ms → `EN` 拉高 → `BOOT` 恢复低
- **flash**(进入下载模式,顺序敏感):
  1. 拉低 `EN`(进入复位,200ms)
  2. 拉高 `BOOT`(下载模式电平,100ms)
  3. 释放 `EN`(上升沿采样 `BOOT`=高 → 进入下载模式,200ms)
  4. 释放 `BOOT`(回到空闲态)

> 注意:进入下载模式后,还需要上位机通过被控板 UART 发送握手(如 `0x55`),否则可能超时退回正常启动。

## 示例输出

```text
[mqtt] connected to 192.168.31.121:1883 rc=0
[mqtt] SEND wb2/control: reboot
[mqtt] RCV wb2/status: 'OK:reboot'
[ok] ack: OK:reboot
```

## 故障排查

| 现象 | 可能原因 |
|------|----------|
| 无回执 / `[fail]` | 控制板未启动、未连上 WiFi、或 broker 地址不对 |
| 连接 broker 失败 | 网络不通,检查 `--broker`/`--port` |
| 控制板反复连 WiFi 失败 | 板子 env 中 SSID/密码有误,用 `set_env` 重新配置 |
| 被控板"只重启、不进下载模式" | 确认 `flash` 四步时序与 BOOT 极性是否符合被控板 |
| 控制板卡 bootrom(2M sync 回全零) | 拔电 5 秒再插回,只按 RST;必要时按住 BOOT+按 RST 进下载模式重烧 |
