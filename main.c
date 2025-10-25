#include <stdio.h>
#include "htu21d.h"
#include "bmp280.h"
#include "db.h"

#define I2C_BUS "/dev/i2c-1"
#define DB_FILE "/var/lib/rpi_sensors_data/data.db"
#define DB_DATA_SIZE 100

// Function to handle sensor reading and storage
void sensors_update(struct bmp280 *bmp280_sens,
                    struct htu21d *htu21d_sens, struct sensors_db *sens_db, struct htu21d_measurement *temperature, struct htu21d_measurement *humidity,
                    float *bmp280_temp, float *bmp280_pressure,
                    int verbose)
{

    if (bmp280_get_measurement(bmp280_sens, bmp280_temp, bmp280_pressure) == 0)
    {
        if (verbose)
        {
            printf("BMP280 temperature: %.2f °C\n", *bmp280_temp);
            printf("BMP280 pressure: %.2f hPa\n", *bmp280_pressure);
        }

        *temperature = htu21d_read_temperature_no_hold(htu21d_sens);
        *humidity = htu21d_read_humidity_no_hold(htu21d_sens);

        if (temperature->is_valid && humidity->is_valid)
        {
            if (verbose)
            {
                printf("HTU21D temperature: %.2f °C\n", temperature->value);
                printf("HTU21D humidity: %.2f %%RH\n", humidity->value);
            }

            if (sensors_db_store_data(sens_db, *bmp280_temp, *bmp280_pressure,
                                      temperature->value, humidity->value) == 0)
            {
                if (verbose)
                    printf("Sensors data stored successfully\n");
            }
        }
        else if (verbose)
        {
            printf("Invalid HTU21D data: temp valid = %d, humidity valid = %d\n",
                   temperature->is_valid, humidity->is_valid);
        }
    }
}

int main(int argc, char *argv[])
{
    struct I2cBus *i2c_bus = i2c_init(I2C_BUS);

    if (!i2c_bus)
    {
        perror("Failed to initialize I2C bus");
        return -1;
    }

    // Initialize sensors
    struct bmp280 *bmp280_sens = bmp280_init(i2c_bus);
    float bmp280_temp, bmp280_pressure;

    struct htu21d *htu21d_sens = htu21d_init(i2c_bus);
    struct htu21d_measurement temperature, humidity;

    // Initialize SQLite database
    struct sensors_db *sens_db = sensors_db_init(DB_FILE, DB_DATA_SIZE);

    // Main measurement loop
    while (1)
    {
        sensors_update(bmp280_sens, htu21d_sens, sens_db, &temperature, &humidity, &bmp280_temp, &bmp280_pressure, verbose);

        sleep(5); // Wait 5 seconds between measurements
    }
    i2c_close(i2c_bus);
    bmp280_close(bmp280_sens);
    htu21d_close(htu21d_sens);
    sensors_db_close(sens_db);

    return 0;
}
