#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include <aos/yloop.h>
#include "blog.h"
#include <hosal_gpio.h>

#include <lwip/tcpip.h>
#include "radar_handler.h"
#include "store.h"
#include "wifi_sta.h"
#include "blufi_app.h"
#include "ha_device.h"
#include "ha_push.h"
#include "ha_mdns.h"
#include "app_config.h"
#include <wifi_mgmr_ext.h>

#include "platform.h"
#include "config.h"
#include "banyan.h"
#include "dataprocess.h"
#include "cmdprocess.h"
#include "bsp_uart.h"
#include "bsp_spi.h"
#include "mcu_sleep.h"
#include <timers.h>

TimerHandle_t spi_timeout_handle = NULL;

static const ha_device_t ha_dev = {
    .type = DEVICE_TYPE,
    .name = DEVICE_NAME,
    .port = TCP_SERVER_PORT,
    .get_state = radar_handler_get_state,
    .set_state = radar_handler_set_state,
};

static void on_got_ip(void)
{
    store_reboot_provision_clear();
    blog_info("[APP] got ip, starting tcp json server");
    blog_info("[SYS] Memory left is %d Bytes", xPortGetFreeHeapSize());
    ha_push_init();
    ha_mdns_start();
    xTaskCreate(ha_tcp_server_start, (char *)"tcp_json", TCP_SERVER_STACK, (void *)&ha_dev, 15, NULL);
}

static void app_evt_cb(input_event_t *event, void *private_data)
{
    switch (event->code) {
    case CODE_WIFI_ON_GOT_IP:
        on_got_ip();
        break;
    default:
        break;
    }
}

static void (*original_radar_callback)(uint8_t *buff, uint16_t len) = NULL;

#define PROTOCOL_HEAD  0xF1F2F3F4
#define PROTOCOL_TAIL  0xF5F6F7F8

static uint8_t parse_radar_motion(uint8_t *buf, uint16_t len)
{
    if (len < 16)
        return 0;

    uint32_t head = *(uint32_t*)&buf[0];
    if (head != PROTOCOL_HEAD)
        return 0;

    uint32_t tail = *(uint32_t*)&buf[len - 4];
    if (tail != PROTOCOL_TAIL)
        return 0;

    return buf[8];
}

static void radar_data_forward(uint8_t *buff, uint16_t len)
{
    uint8_t motion = parse_radar_motion(buff, len);
    uint8_t old_motion = radar_handler_get_motion();

    radar_handler_set_motion(motion);

    if (motion != old_motion && ha_push_enabled()) {
        uint8_t mac[6];
        char dev[192];
        char full_json[256];

        radar_handler_get_state(dev, sizeof(dev));
        if (wifi_mgmr_sta_mac_get(mac) != 0) {
            memset(mac, 0, sizeof(mac));
        }
        snprintf(full_json, sizeof(full_json),
                 "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
                 "\"type\":\"%s\",\"name\":\"%s\",%s}",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                 DEVICE_TYPE, DEVICE_NAME, dev);
        ha_push_send(full_json);
    }

    if (original_radar_callback) {
        original_radar_callback(buff, len);
    }
}

static void hook_radar_callback(void)
{
    extern void (*SendResultCallback)(uint8_t *buff, uint16_t len);
    original_radar_callback = SendResultCallback;
    SendResultCallback = radar_data_forward;
}

static hosal_gpio_dev_t cs_io;
static EventGroupHandle_t SPIEventGroup;
#define EVT_GROUP_SPI_FLAG    (1<<1)

EventGroupHandle_t CmdProcessEventGroup;

static void cmd_process_task(void *param)
{
    EventBits_t uxBits;

    for (;;)
    {
        uxBits = xEventGroupWaitBits(CmdProcessEventGroup, CMD_PROCESS_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

        if( ( uxBits & CMD_PROCESS_BIT ) != 0 )
        {
            CmdProc_Recv();
        }
    }
}

void cs_io_irq(void *arg)
{
    BaseType_t xResult = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (SPIEventGroup != NULL)
    {
        xResult = xEventGroupSetBitsFromISR(SPIEventGroup, EVT_GROUP_SPI_FLAG, &xHigherPriorityTaskWoken);
        if(xResult != pdFAIL)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void cs_interrupt_init(void)
{
    cs_io.port = 22;
    cs_io.config = INPUT_PULL_UP;
    hosal_gpio_init(&cs_io);
    hosal_gpio_irq_set(&cs_io, HOSAL_IRQ_TRIG_NEG_PULSE, cs_io_irq, NULL);
}

#define SPI_INII_STACK_SIZE (512)
StaticTask_t xInitTaskBuffer;
StackType_t xInitStack[ SPI_INII_STACK_SIZE ];

void spi_init_task(void *param)
{
    EventBits_t uxBits;
    void (*func_init)(void) = param;

    for(;;)
    {
        uxBits = xEventGroupWaitBits(SPIEventGroup, EVT_GROUP_SPI_FLAG, pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & EVT_GROUP_SPI_FLAG)
        {
            func_init();
            break;
        }
    }

    vTaskDelete(NULL);
}

void create_spi_init_task(void (*spi_slave_init_type)(void))
{
    TaskHandle_t handle;
    handle = xTaskCreateStatic(
                            spi_init_task,
                            "spi_init_t",
                            SPI_INII_STACK_SIZE,
                            spi_slave_init_type,
                            configMAX_PRIORITIES-1,
                            xInitStack,
                            &xInitTaskBuffer);

    if (handle == NULL)
    {
        printf("create spi_init_task failed\r\n");
    }
}

static void boot_mode(void)
{
    store_wifi_t cfg;

    store_init();
    aos_register_event_filter(EV_WIFI, app_evt_cb, NULL);

    if (store_reboot_provision_check(REBOOT_PROVISION_COUNT)) {
        blog_info("[APP] %d consecutive reboots, enter provisioning mode", REBOOT_PROVISION_COUNT);
        store_reboot_provision_clear();
        blufi_app_start();
        return;
    }

    if (store_wifi_load(&cfg)) {
        blog_info("[APP] wifi ssid: %s", cfg.ssid);
        blog_info("[APP] connecting wifi");
        wifi_sta_start(cfg.ssid, cfg.pwd);
    } else {
        blog_info("[APP] no wifi config, enter blufi provisioning");
        store_reboot_provision_clear();
        blufi_app_start();
    }
}

int main(void)
{
    bsp_uart0_init();
    Platform_Init();

    Config_Init();

    CmdProcessEventGroup = xEventGroupCreate();
    SPIEventGroup = xEventGroupCreate();

    extern QueueHandle_t SpiDataQueue;
    SpiDataQueue = xQueueCreate(FUNC_QUEUE_SIZE, 48);

    vTaskDelay(pdMS_TO_TICKS(3));
    Radar_Init();

    xTaskCreate(cmd_process_task, "cmd_pro", 512, NULL, CMD_PROCESS_TASK_PRIORITY, NULL);

    DataProc_Init();
    CmdProc_Init();

    extern void queue_data_process_task(void *param);
    xTaskCreate(queue_data_process_task, "data_pro", 512, NULL, DATA_PROCESS_TASK_PRIORITY, NULL);
    cs_interrupt_init();
    create_spi_init_task(bsp_spi_slave_init);

    xTaskCreate(uart_recv_timeout_check_task, "uart_rec_check", 128, NULL, UART_RECV_CHECK_TASK_PRIORITY, NULL);

    hook_radar_callback();

    puts("[OS] radar_rd_01 starting...");
    tcpip_init(NULL, NULL);
    boot_mode();
    puts("[OS] main exit");

    return 0;
}
