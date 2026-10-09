#include "tw_tts_app.h"
#include "tw_tts_gb2312.h"

/*
 * CITW-TTS 语音合成芯片串口协议实现
 * 帧格式：0xFD | 0x00 LL | 命令字 | 编码参数 | 文本
 *   LL = 命令字之后的字节数（编码参数 + 文本），串口 9600-8-N-1
 *
 * 文本一律由 UTF-8 转成 GB2312（编码参数 0x00）再发：
 *   1) 手册示例全部基于 GB2312，兼容性最好
 *   2) GB2312 汉字 2 字节，比 UTF-8 3 字节省 1/3 线上时间
 * 整帧（帧头+文本）拼进同一块缓冲后一次性 uart1_send，不在帧中间产生发送间隔。
 */

/* 短控制帧（参数/提示音/停止等）payload 上限，够放 "message_5" 等 */
#define TTS_CTRL_PAYLOAD_MAX  32

static void tts_send_ctrl_frame(uint8_t cmd, uint8_t enc, int has_enc,
                                const uint8_t *payload, uint32_t payload_len)
{
    uint8_t frame[5 + TTS_CTRL_PAYLOAD_MAX];
    uint32_t ll = payload_len + (has_enc ? 2 : 1);
    uint32_t index = 0;

    if (payload_len > TTS_CTRL_PAYLOAD_MAX) {
        return;
    }
    frame[index++] = TTS_FRAME_HEAD;
    frame[index++] = (uint8_t)(ll >> 8);
    frame[index++] = (uint8_t)(ll & 0xFF);
    frame[index++] = cmd;
    if (has_enc) {
        frame[index++] = enc;
    }
    if (payload_len) {
        memcpy(&frame[index], payload, payload_len);
        index += payload_len;
    }
    uart1_send(frame, (uint16_t)index);      /* 整帧一次发出 */
}

/* 无文本的控制帧：FD 00 01 cmd */
static void tts_send_ctrl(uint8_t cmd)
{
    tts_send_ctrl_frame(cmd, 0, 0, NULL, 0);
}

/* UTF-8 文本按多字节边界截断，避免切碎汉字 */
static uint32_t tts_clip_utf8(const char *text, uint32_t max_len)
{
    uint32_t len = (uint32_t)strlen(text);

    if (len <= max_len) {
        return len;
    }
    len = max_len;
    while (len > 0 && ((uint8_t)text[len] & 0xC0) == 0x80) {
        len--;
    }
    uart_log("[TTS] text too long, clipped\r\n");
    return len;
}

void TTS(const char *chinese)
{
    /* 整帧缓冲用 static：TCP 任务单独调用，避免 4KB 栈占用 */
    static uint8_t frame[5 + TTS_MAX_TEXT_BYTES + 4];
    uint32_t len;
    uint32_t ll;
    int gb_len;

    if (chinese == NULL) {
        return;
    }
    len = tts_clip_utf8(chinese, TTS_MAX_TEXT_BYTES);
    if (len == 0) {
        return;
    }

    /* UTF-8 -> GB2312，直接落进帧缓冲的文本区 */
    gb_len = tw_utf8_to_gb2312((const uint8_t *)chinese, (int)len,
                               &frame[5], (int)sizeof(frame) - 6);
    if (gb_len <= 0) {
        /* 转不出有效字符（如全是生僻字/emoji），退回 UTF-8 直发 */
        uart_log("[TTS] gb2312 convert empty, fallback utf8\r\n");
        if (len > sizeof(frame) - 6) {
            len = sizeof(frame) - 6;
        }
        memcpy(&frame[5], chinese, len);
        gb_len = (int)len;
        ll = gb_len + 2;
        frame[0] = TTS_FRAME_HEAD;
        frame[1] = (uint8_t)(ll >> 8);
        frame[2] = (uint8_t)(ll & 0xFF);
        frame[3] = TTS_CMD_START;
        frame[4] = TTS_ENC_UTF8;
        uart1_send(frame, (uint16_t)(5 + gb_len));
        return;
    }

    ll = (uint32_t)gb_len + 2;
    frame[0] = TTS_FRAME_HEAD;
    frame[1] = (uint8_t)(ll >> 8);
    frame[2] = (uint8_t)(ll & 0xFF);
    frame[3] = TTS_CMD_START;
    frame[4] = TTS_ENC_GB2312;
    uart1_send(frame, (uint16_t)(5 + gb_len));    /* 整帧一次发出 */
}

/* 参数标记帧：FD 00 06 01 01 '[' tag '0'+value ']'，并语音播报确认 */
static void tts_set_param(uint8_t tag, uint8_t value, const char *feedback)
{
    uint8_t payload[4];
    char num[2];

    if (value > TTS_PARAM_MAX) {
        return;
    }

    payload[0] = '[';
    payload[1] = tag;
    payload[2] = (uint8_t)('0' + value);
    payload[3] = ']';
    tts_send_ctrl_frame(TTS_CMD_START, TTS_ENC_CTRL, 1, payload, sizeof(payload));

    vTaskDelay(100);
    TTS(feedback);
    vTaskDelay(200);
    num[0] = (char)('0' + value);
    num[1] = '\0';
    TTS(num);
    vTaskDelay(2000);
}

void TTS_set_volume(uint8_t volume)
{
    tts_set_param('v', volume, "\xE9\x9F\xB3\xE9\x87\x8F\xE8\xAE\xBE\xE7\xBD\xAE\xE4\xB8\xBA");
}

void TTS_set_speed(uint8_t speed)
{
    tts_set_param('s', speed, "\xE8\xAF\xAD\xE9\x80\x9F\xE8\xAE\xBE\xE7\xBD\xAE\xE4\xB8\xBA");
}

void TTS_set_tone(uint8_t tone)
{
    tts_set_param('t', tone, "\xE8\xAF\xAD\xE8\xB0\x83\xE8\xAE\xBE\xE7\xBD\xAE\xE4\xB8\xBA");
}

void TTS_stop(void)
{
    tts_send_ctrl(TTS_CMD_STOP);
}

void TTS_pause(void)
{
    tts_send_ctrl(TTS_CMD_PAUSE);
}

void TTS_resume(void)
{
    tts_send_ctrl(TTS_CMD_RESUME);
}

void TTS_query_status(void)
{
    tts_send_ctrl(TTS_CMD_QUERY);
}

/* 提示音帧：FD 00 LL 01 01 <名称>，如 ring_1 / message_3 / alert_5 */
static void tts_play_named(const char *name, uint8_t index)
{
    char sound[16];

    if (index < 1 || index > 5) {
        return;
    }
    snprintf(sound, sizeof(sound), "%s_%u", name, (unsigned)index);
    tts_send_ctrl_frame(TTS_CMD_START, TTS_ENC_CTRL, 1,
                        (const uint8_t *)sound, (uint32_t)strlen(sound));
}

void TTS_play_ring(uint8_t index)
{
    tts_play_named("ring", index);
}

void TTS_play_message(uint8_t index)
{
    tts_play_named("message", index);
}

void TTS_play_alert(uint8_t index)
{
    tts_play_named("alert", index);
}
