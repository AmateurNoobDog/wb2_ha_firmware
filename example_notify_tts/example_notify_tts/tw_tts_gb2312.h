#ifndef TW_TTS_GB2312_H
#define TW_TTS_GB2312_H
#include <stdint.h>

/* 由 Python gb2312 编码表生成：unicode 码点 -> GB2312 双字节(高字节在前) */
typedef struct { uint16_t uni; uint16_t gb; } gb2312_map_t;
extern const gb2312_map_t gb2312_map[];
extern const int gb2312_map_size;

/* 返回 GB2312 字节数；非 GB2312 字符返回 0，ASCII 原样返回 1 */
int tw_utf8_to_gb2312(const uint8_t *utf8, int len, uint8_t *out, int out_max);

#endif
