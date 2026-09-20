# ha_lib — Home Assistant 共享库

设备无关的 TCP JSON 服务器库，供所有 HA 固件项目（light/switch/radar_rd_01/433_gateway/usb_sw/tts）复用。

## 文件说明

| 文件 | 说明 |
|------|------|
| `ha_device.h` | 设备抽象结构体 `ha_device_t`，定义设备类型/名称/型号/厂商/版本/端口/回调 |
| `ha_json.c/h` | 基于 cJSON 的 JSON 字段解析器（`ha_json_str`/`ha_json_int`） |
| `tcp_json_server.c` | TCP 服务主逻辑：监听端口、响应 `get_device`/`get_state`、分发 `set` |
| `ha_push.c/h` | 推送模块（NVS 配置目标主机/端口 + TCP 推送） |
| `ha_mdns_query.c/h` | 域名解析（.local 走 mDNS 组播查询，其余走 DNS，带 4 条缓存） |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## API

### ha_device_t（设备抽象）

```c
typedef struct {
    const char *type;                     // 设备类型，如 "light"/"switch"/"radar"/"event"/"tts"
    const char *name;                     // 设备名称，如 "彩灯"/"智能开关"/"433网关"
    const char *model;                    // 设备型号，如 "Ai-WB2-12F"/"RD-01"
    const char *manufacturer;             // 厂商，如 "AND-DIY"
    const char *sw_version;               // 固件版本，如 "1.0.1"
    int port;                             // TCP 监听端口，默认 9100
    int (*get_device)(char *buf, int buf_len);  // 填充实体定义（不含花括号）
    int (*get_state)(char *buf, int buf_len);   // 填充实体状态（不含花括号）
    int (*set_state)(const char *cmd_json);     // 解析并执行 set 命令
    void (*log)(const char *msg);               // 可选：日志输出回调
} ha_device_t;
```

### ha_tcp_server_start()

```c
void ha_tcp_server_start(void *pvParameters);
```

TCP 服务器入口，pvParameters 必须指向一个持久有效的 `const ha_device_t`。
在 FreeRTOS 中以 task 方式运行，默认栈大小 4096。

工作流程：
1. 监听 `ha_device_t.port` 端口
2. 收到 `{"cmd":"get_device"}` 时，调用 `get_device()` 返回实体定义
3. 收到 `{"cmd":"get_state"}` 时，调用 `get_state()` 返回实体状态
4. 收到 `{"cmd":"set",...}` 时，调用 `set_state()` 执行控制

### ha_json_str / ha_json_int

```c
const char *ha_json_str(const char *buf, const char *key);
int ha_json_int(const char *buf, const char *key, int def);
```

基于 cJSON 库的 JSON 字段提取，用于解析命令。
`ha_json_str` 返回的字符串以 `\0` 结尾，可安全用于 `strcmp`。

### ha_push 模块

```c
void ha_push_init(void);                                    // 从 NVS 加载配置
void ha_push_set_target(const char *host, uint16_t port);   // 设置目标主机/IP 并保存到 NVS
void ha_push_clear(void);                                   // 清除推送配置并禁用推送
void ha_push_send(const char *json);                        // 发送 JSON 到 HA（阻塞，同步 TCP 连接）
uint8_t ha_push_enabled(void);                              // 是否已配置目标
```

`ha_push_set_target` 的 `host` 参数支持 IP 地址（如 `"192.168.1.100"`）和域名（如 `"homeassistant.local"`），
域名通过 `ha_mdns_query` 模块自动解析（.local 走 mDNS，其余走 DNS）。

## 新设备接入步骤

1. 在 `app_config.h` 中定义 `DEVICE_TYPE`/`DEVICE_NAME`/`DEVICE_MODEL`/`DEVICE_MANUFACTURER`/`DEVICE_SW_VERSION`/`TCP_SERVER_PORT`
2. 实现 `xxx_handler_get_device()`、`xxx_handler_get_state()` 和 `xxx_handler_set_state()` 函数
3. 在 `main.c` 中注册 `ha_device_t` 并启动服务器：

```c
static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .model = DEVICE_MODEL,
    .manufacturer = DEVICE_MANUFACTURER,
    .sw_version = DEVICE_SW_VERSION,
    .port = TCP_SERVER_PORT,
    .get_device = xxx_handler_get_device,
    .get_state = xxx_handler_get_state,
    .set_state = xxx_handler_set_state,
};

// WiFi 获取 IP 后启动
xTaskCreate(ha_tcp_server_start, "tcp_json", TCP_SERVER_STACK, (void *)&ha_dev, 15, NULL);
```

## 协议格式（v2）

### get_device — 返回实体定义

```
请求: {"cmd":"get_device"}
响应: {"mac":"XX:XX:XX:XX:XX:XX","name":"...","model":"...","manufacturer":"...","sw_version":"...",
       "entities":[{"id":"...","type":"...","name":"...","icon":"..."}]}
```

### get_state — 返回实体状态

```
请求: {"cmd":"get_state"}
响应: {"state":"online","entities":[{"id":"...","type":"...","on":1}]}
```

### set — 控制实体

```
请求: {"cmd":"set","id":"XXX_NNN",...}
响应: {"state":"online","entities":[...]}
```

## 实体类型

| 类型 | 有 command | 有 state | 说明 |
|------|-----------|----------|------|
| `light` | set | 有 | RGB 灯 |
| `switch` | set | 有 | 继电器开关 |
| `binary_sensor` | — | 有 | 二进制传感器 |
| `button` | action | — | 按钮 |
| `event` | — | 有 | 事件 |
| `sensor` | — | 有 | 数据传感器 |
| `notify` | set（text） | — | 通知（TTS 语音合成） |
| `number` | set（value） | 有 | 数值（TTS 音量/语速） |
