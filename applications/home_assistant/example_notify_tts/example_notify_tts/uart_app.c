#include "uart_app.h"
#include "app_config.h"

hosal_uart_dev_t uart_0 = {
    .config = {
        .uart_id = 0,
        .tx_pin = UART_DEBUG_TX_PIN,
        .rx_pin = UART_DEBUG_RX_PIN,
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = UART_DEBUG_BAUDRATE,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity = HOSAL_NO_PARITY,
        .stop_bits = HOSAL_STOP_BITS_1,
        .mode = HOSAL_UART_MODE_POLL,
    },
};

hosal_uart_dev_t uart_1 = {
    .config = {
        .uart_id = 1,
        .tx_pin = UART_TTS_TX_PIN,
        .rx_pin = UART_TTS_RX_PIN,
        .cts_pin = 255,
        .rts_pin = 255,
        .baud_rate = UART_TTS_BAUDRATE,
        .data_width = HOSAL_DATA_WIDTH_8BIT,
        .parity = HOSAL_NO_PARITY,
        .stop_bits = HOSAL_STOP_BITS_1,
        .mode = HOSAL_UART_MODE_POLL,
    },
};

void uart_init(){
    hosal_uart_init(&uart_0);
    hosal_uart_init(&uart_1);
}

void uart0_send(uint8_t *data, uint16_t length){
    hosal_uart_send(&uart_0, data, length);
}

uint16_t uart0_get(uint8_t *data){
    uint16_t length = hosal_uart_receive(&uart_0, data, sizeof(data));
    return length;
}

void uart0_print(char* data){
    int len = 0;
    while (1){
        if(data[len]=='\0'){
            break;
        }else{
            len++;
        }
    }
    hosal_uart_send(&uart_0, data, len);
}

void uart1_send(uint8_t *data, uint16_t length){
    hosal_uart_send(&uart_1, data, length);
}

uint16_t uart1_get(uint8_t *data){
    uint16_t length = hosal_uart_receive(&uart_1, data, sizeof(data));
    return length;
}

void uart1_print(char* data){
    int len = 0;
    while (1){
        if(data[len]=='\0'){
            break;
        }else{
            len++;
        }
    }
    hosal_uart_send(&uart_1, data, len);
}

void uart_log(const char *msg) {
    if (msg) {
        int len = 0;
        while (msg[len] != '\0') {
            len++;
        }
        hosal_uart_send(&uart_0, (uint8_t *)msg, len);
    }
}
