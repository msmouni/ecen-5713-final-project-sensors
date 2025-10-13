#include <stdio.h>
#include "htu21d.h"
#include "bmp280.h"

#define I2C_BUS "/dev/i2c-1"

int main()
{
    struct I2cBus *i2c_bus = i2c_init(I2C_BUS);

    if (!i2c_bus)
    {
        return -1;
    }

    // BMP280
    struct bmp280 *bmp280_sens = bmp280_init(i2c_bus);
    float bmp280_temp, bmp280_pressure;
    if (bmp280_get_measurement(bmp280_sens, &bmp280_temp, &bmp280_pressure) == 0)
    {
        printf("BMP280 temperature: %.2f °C\n", bmp280_temp);
        printf("BMP280 pressure: %.2f hPa\n", bmp280_pressure);
    }
    else
    {
        printf("Error reading BMP280 measurement\n");
    }

    // HTU21D
    struct htu21d *htu21d_sens = htu21d_init(i2c_bus);
    struct htu21d_measurement temperature = htu21d_read_temperature(htu21d_sens);
    struct htu21d_measurement humidity = htu21d_read_humidity(htu21d_sens);

    if (temperature.is_valid && humidity.is_valid)
    {
        printf("Temperature: %.2f °C\n", temperature.value);
        printf("Humidity: %.2f %%RH\n", humidity.value);
    }
    else
    {
        printf("Invalid measurement: humidity : %d | temperature : %d \n", humidity.is_valid, temperature.is_valid);
    }

    i2c_close(i2c_bus);
    bmp280_close(bmp280_sens);
    htu21d_close(htu21d_sens);

    return 0;
}
