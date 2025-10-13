#ifndef BMP280_H
#define BMP280_H

#include "i2c.h"

// Calibration parameters structure
typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
} bmp280_calib_data;

struct bmp280
{
    bmp280_calib_data calib;
    struct I2cBus *i2c_bus;
};

struct bmp280 *bmp280_init(struct I2cBus *i2c_bus);

// Function to calculate temperature and pressure using the calibration parameters
int bmp280_get_measurement(struct bmp280 *self,
                           float *temperature, float *pressure);

void bmp280_close(struct bmp280 *self);

#endif /* BMP280_H */
