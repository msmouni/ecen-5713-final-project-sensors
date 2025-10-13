#ifndef HTU21_D_H
#define HTU21_D_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

struct htu21d_measurement
{
    bool is_valid;
    float value;
};

struct htu21d
{
    struct I2cBus *i2c_bus;
};

struct htu21d *htu21d_init(struct I2cBus *i2c_bus);
struct htu21d_measurement htu21d_read_temperature(struct htu21d *self);
struct htu21d_measurement htu21d_read_humidity(struct htu21d *self);
void htu21d_close(struct htu21d *self);

#endif /* HTU21_D_H */
