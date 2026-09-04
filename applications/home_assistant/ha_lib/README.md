# ha_lib — Home Assistant 共享库

设备无关的 TCP JSON 服务器库,供所有 HA 固件项目(light/switch/radar_rd_01)复用。

## 文件说明

| 文件 | 说明 |
|------|------|
| `ha_device.h` | 设备抽象结构体 `ha_device_t`,定义设备类型/名称/端口/回调 |
| `ha_json.c/h` | 极简 JSON 字段解析器(`ha_json_str`/`ha_json_int`) |
| `tcp_json_server.c` | TCP 服务主逻辑:监听端口、响应状态、分发 `cmd:set` |
| `ha_push.c/h` | 推送模块(NVS 配置目标 IP/端口 + TCP 推送) |
| `bouffalo.mk` | BL602 SDK 组件构建脚本 |

## API

### ha_device_t(设备抽象)

```c
typedef struct {
    const char *type;                     // 设备类型,如 "wb2"/"sw"/"radar"
    const char *name;                     // 设备名称,如 "彩灯"/"智能开关"/"雷达"
    int port;                             // TCP 监听端口,默认 9100
    int (*get_state)(char *buf, int buf_len);   // 填充设备状态字段(不含花括号)
    int (*set_state)(const char *cmd_json);     // 解析并执行 "cmd":"set" 请求
} ha_device_t;
```

### ha_tcp_server_start()

```c
void ha_tcp_server_start(void *pvParameters);
```

TCP 服务器入口,pvParameters 必须指向一个持久有效的 `const ha_device_t`。
在 FreeRTOS 中以 task 方式运行,默认栈大小 4096。

工作流程:
1. 监听 `ha_device_t.port` 端口
2. 收到 `{"cmd":"get"}` 时,调用 `get_state()` 拼接响应
3. 收到 `{"cmd":"set",...}` 时,调用 `set_state()` 执行控制
4. 响应格式: `{"mac":"XX:XX:XX:XX:XX:XX","type":"...","name":"...",<设备字段>}`

### ha_json_str / ha_json_int

```c
const char *ha_json_str(const char *buf, const char *key);
int ha_json_int(const char *buf, const char *key, int def);
```

从 JSON 字符串中提取字段值,用于解析 `set_state` 命令。

### ha_push 模块

```c
void ha_push_init(void);                              // 从 NVS 加载配置
void ha_push_set_target(const char *ip, uint16_t port); // 设置目标并保存到 NVS
void ha_push_send(const char *json);                  // 发送 JSON 到 HA(非阻塞)
uint8_t ha_push_enabled(void);                        // 是否已配置目标
```

## 新设备接入步骤

1. 在 `app_config.h` 中定义 `DEVICE_TYPE`/`DEVICE_NAME`/`DEVICE_MODEL`/`TCP_SERVER_PORT`
2. 实现 `xxx_handler_get_state()` 和 `xxx_handler_set_state()` 函数
3. 在 `main.c` 中注册 `ha_device_t` 并启动服务器:

```c
static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .port = TCP_SERVER_PORT,
    .get_state = radar_handler_get_state,
    .set_state = radar_handler_set_state,
};

// WiFi 获取 IP 后启动
xTaskCreate(ha_tcp_server_start, "tcp_json", TCP_SERVER_STACK, (void *)&ha_dev, 15, NULL);
```

## 协议格式

所有设备共享相同的顶层 JSON 结构,设备特定字段由 `get_state()` 回调填充:

```
请求: {"cmd":"get"}  或  {"cmd":"set",<字段>}
响应: {"mac":"XX:XX:XX:XX:XX:XX","type":"...","name":"...",<设备字段>}\r\n
```
