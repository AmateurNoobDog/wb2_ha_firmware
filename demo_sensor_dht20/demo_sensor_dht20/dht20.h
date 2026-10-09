#ifndef __DHT20_H__
#define __DHT20_H__

/*
 * DHT20 temperature/humidity sensor driver.
 * I2C interface, address 0x38.
 */

/* Initialize DHT20 sensor (I2C + startup delay).
 * Returns 0 on success. */
int DHT20_Init(void);

/* Read temperature and humidity.
 * Returns 0 on success, -1 on CRC error or I2C failure. */
int DHT20_Read(float *temperature, float *humidity);

#endif /* __DHT20_H__ */
