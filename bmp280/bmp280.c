#include "bmp280.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "stdlib.h"

#define BMP280_ADDR 0x76

// BMP280 registers
#define REG_CALIB 0x88
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG 0xF5
#define REG_PRESS_MSB 0xF7
#define REG_TEMP_MSB 0xF7

// Function to read and parse BMP280 calibration data
static int
bmp280_read_calibration(struct I2cBus *i2c_bus, bmp280_calib_data *calib)
{
    if (i2c_bus == NULL || calib == NULL)
    {
        return -1;
    }

    uint8_t calib_data[24];
    if (i2c_read_register(i2c_bus, BMP280_ADDR, REG_CALIB, calib_data, 24) < 0)
    {
        return -1;
    }

    calib->dig_T1 = (calib_data[1] << 8) | calib_data[0];
    calib->dig_T2 = (calib_data[3] << 8) | calib_data[2];
    calib->dig_T3 = (calib_data[5] << 8) | calib_data[4];
    calib->dig_P1 = (calib_data[7] << 8) | calib_data[6];
    calib->dig_P2 = (calib_data[9] << 8) | calib_data[8];
    calib->dig_P3 = (calib_data[11] << 8) | calib_data[10];
    calib->dig_P4 = (calib_data[13] << 8) | calib_data[12];
    calib->dig_P5 = (calib_data[15] << 8) | calib_data[14];
    calib->dig_P6 = (calib_data[17] << 8) | calib_data[16];
    calib->dig_P7 = (calib_data[19] << 8) | calib_data[18];
    calib->dig_P8 = (calib_data[21] << 8) | calib_data[20];
    calib->dig_P9 = (calib_data[23] << 8) | calib_data[22];

    return 0;
}

struct bmp280 *bmp280_init(struct I2cBus *i2c_bus)
{
    if (!i2c_bus)
    {
        goto err_exit;
    }

    struct bmp280 *sens = (struct bmp280 *)malloc(sizeof(struct bmp280));

    if (!sens)
    {
        goto err_exit;
    }

    sens->i2c_bus = i2c_bus;

    if (bmp280_read_calibration(i2c_bus, &sens->calib) < 0)
    {
        goto err_free;
    }

    // Configure BMP280: Normal mode, temperature + pressure, oversampling x1, and standby time 0.5ms
    if (i2c_write_register(i2c_bus, BMP280_ADDR, REG_CTRL_MEAS, 0x27) != 0)
    {
        perror("Failed to configure BMP280");
        goto err_free;
    }

    return sens;

err_free:
    free(sens);
err_exit:
    return NULL;
}

// Function to calculate temperature and pressure using the calibration parameters
int bmp280_get_measurement(struct bmp280 *self,
                           float *temperature, float *pressure)
{
    if (self == NULL || self->i2c_bus == NULL || temperature == NULL || pressure == NULL)
    {
        return -1;
    }

    uint8_t data[6];

    // Read 6 bytes: 3 bytes for pressure and 3 bytes for temperature
    if (i2c_read_register(self->i2c_bus, BMP280_ADDR, REG_PRESS_MSB, data, 6) != 0)
    {
        *temperature = 0;
        *pressure = 0;
        return -1;
    }

    // Extract raw ADC values from data array
    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);

    // Temperature calculations (from BMP280 datasheet)
    int32_t var1, var2, t_fine;

    var1 = ((((adc_T >> 3) - ((int32_t)self->calib.dig_T1 << 1))) * ((int32_t)self->calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)self->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)self->calib.dig_T1))) >> 12) *
            ((int32_t)self->calib.dig_T3)) >>
           14;
    t_fine = var1 + var2;
    *temperature = (t_fine * 5 + 128) >> 8;
    *temperature /= 100.0; // Convert to °C

    // Pressure calculations (from BMP280 datasheet)
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)self->calib.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)self->calib.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)self->calib.dig_P4) << 16);
    var1 = (((self->calib.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)self->calib.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)self->calib.dig_P1)) >> 15);

    if (var1 == 0)
    {
        *pressure = 0; // Avoid division by zero
        return 0;
    }

    int32_t p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
    p = (p < (int32_t)0x80000000) ? (p << 1) / ((uint32_t)var1) : (p / (uint32_t)var1) * 2;
    var1 = (((int32_t)self->calib.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)self->calib.dig_P8)) >> 13;
    p = (int32_t)((int32_t)p + ((var1 + var2 + self->calib.dig_P7) >> 4));

    *pressure = p / 100.0; // Convert to hPa

    return 0;
}

void bmp280_close(struct bmp280 *self)
{
    free(self);
}
