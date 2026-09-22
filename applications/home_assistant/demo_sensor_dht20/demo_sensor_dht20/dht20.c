#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <blog.h>
#include <hosal_i2c.h>

#include "dht20.h"
#include "app_config.h"

#define DHT20_I2C_ADDR      0x38
#define DHT20_CMD_MEASURE   0xAC, 0x33, 0x00
#define DHT20_DATA_LEN      7
#define DHT20_TIMEOUT_MS    100

static hosal_i2c_dev_t s_i2c;

static uint8_t dht20_crc8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc <<= 1;
                crc ^= 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

static int dht20_read_raw(uint8_t *buf, uint8_t len)
{
    uint8_t cmd[3] = { DHT20_CMD_MEASURE };

    if (hosal_i2c_master_send(&s_i2c, DHT20_I2C_ADDR, cmd, sizeof(cmd), DHT20_TIMEOUT_MS) != 0) {
        return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(80));

    int ready = 0;
    for (int retry = 0; retry < 50; retry++) {
        uint8_t status;
        if (hosal_i2c_master_recv(&s_i2c, DHT20_I2C_ADDR, &status, 1, DHT20_TIMEOUT_MS) != 0) {
            return -1;
        }
        if (!(status & 0x80)) {
            ready = 1;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (!ready) {
        blog_error("[DHT20] sensor busy timeout after polling");
        return -1;
    }

    if (hosal_i2c_master_recv(&s_i2c, DHT20_I2C_ADDR, buf, len, DHT20_TIMEOUT_MS) != 0) {
        return -1;
    }

    return 0;
}

int DHT20_Init(void)
{
    s_i2c.config.address_width = HOSAL_I2C_ADDRESS_WIDTH_7BIT;
    s_i2c.config.freq = DHT20_I2C_FREQ;
    s_i2c.config.mode = HOSAL_I2C_MODE_MASTER;
    s_i2c.config.scl = DHT20_I2C_SCL_PIN;
    s_i2c.config.sda = DHT20_I2C_SDA_PIN;
    s_i2c.port = 0;

    int ret = hosal_i2c_init(&s_i2c);
    if (ret != 0) {
        blog_error("[DHT20] i2c init failed: %d", ret);
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
    blog_info("[DHT20] initialized, SCL=%d SDA=%d", DHT20_I2C_SCL_PIN, DHT20_I2C_SDA_PIN);
    return 0;
}

int DHT20_Read(float *temperature, float *humidity)
{
    uint8_t buf[DHT20_DATA_LEN];

    for (int attempt = 0; attempt < 3; attempt++) {
        if (dht20_read_raw(buf, DHT20_DATA_LEN) != 0) {
            blog_warn("[DHT20] raw read failed, attempt %d", attempt + 1);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (dht20_crc8(buf, 6) != buf[6]) {
            blog_warn("[DHT20] crc error attempt %d: calc=0x%02X recv=0x%02X",
                      attempt + 1, dht20_crc8(buf, 6), buf[6]);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        uint32_t raw_hum = buf[1];
        raw_hum <<= 8;
        raw_hum += buf[2];
        raw_hum <<= 4;
        raw_hum += (buf[3] >> 4);
        *humidity = raw_hum * 9.5367431640625e-5f;

        uint32_t raw_temp = (buf[3] & 0x0F);
        raw_temp <<= 8;
        raw_temp += buf[4];
        raw_temp <<= 8;
        raw_temp += buf[5];
        *temperature = raw_temp * 1.9073486328125e-4f - 50.0f;

        if (*humidity > 100.0f || *temperature < -40.0f || *temperature > 80.0f) {
            blog_error("[DHT20] invalid data: temp=%.1f hum=%.1f raw=0x%05X",
                       *temperature, *humidity, raw_hum);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        if (raw_hum == 0 && raw_temp == 0) {
            blog_error("[DHT20] zero raw data detected, sensor may be unresponsive");
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        return 0;
    }

    blog_error("[DHT20] all 3 read attempts failed");
    return -1;
}
