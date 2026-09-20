#include "tw_tts_app.h"

void TTS_set_speed(uint8_t speed){
    uint8_t speed_buf[] = {0xfd,0x00,0x06,0x01,0x01,0x5b,0x73,0x30,0x5d};
    if(speed<=9){
        speed_buf[7] = 0x30 + speed;
        uart1_send(speed_buf, 9);
        vTaskDelay(100);
        TTS("\xE8\xAF\xAD\xE9\x80\x9F\xE8\xAE\xBE\xE7\xBD\xAE\xE4\xB8\xBA");
        vTaskDelay(200);
        char vol_str[2] = { (char)('0' + speed), '\0' };
        TTS(vol_str);
        vTaskDelay(2000);
    }
}

void TTS_set_volume(uint8_t volume){
    uint8_t volume_buf[] = {0xfd,0x00,0x06,0x01,0x01,0x5b,0x76,0x30,0x5d};
    if(volume>=1 && volume<=9){
        volume_buf[7] = 0x30 + volume;
        uart1_send(volume_buf, 9);
        vTaskDelay(100);
        TTS("\xE9\x9F\xB3\xE9\x87\x8F\xE8\xAE\xBE\xE7\xBD\xAE\xE4\xB8\xBA");
        vTaskDelay(200);
        char vol_str[2] = { (char)('0' + volume), '\0' };
        TTS(vol_str);
        vTaskDelay(2000);
    }
}

void TTS(const char *chinese){
    uint8_t frame[1024];
    int index = 0;

    int chinese_len = strlen(chinese);
    frame[index++] = 0xFD;
    frame[index++] = (chinese_len+2)/256;
    frame[index++] = (chinese_len+2)%256;
    frame[index++] = 0x01;
    frame[index++] = 0x04;

    memcpy(&frame[index], chinese, chinese_len);
    index += chinese_len;

    uart1_send(frame, index);
}
