#include <stdio.h>
#include "htu21d.h"

int main()
{
    struct htu21d *htu21d_sens = htu21d_init();

    if (!htu21d_sens)
    {
        return -1;
    }

    struct htu21d_measurement temperature = read_temperature(htu21d_sens);
    struct htu21d_measurement humidity = read_humidity(htu21d_sens);

    if (temperature.is_valid && humidity.is_valid)
    {
        printf("Temperature: %.2f °C\n", temperature.value);
        printf("Humidity: %.2f %%\n", humidity.value);
    }
    else
    {
        printf("Invalid measurement: humidity : %d | temperature : %d ", humidity.is_valid, temperature.is_valid);
    }

    htu21d_close(htu21d_sens);

    return 0;
}
