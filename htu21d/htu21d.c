#include "htu21d.h"
#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "i2c.h"

#define HTU21D_I2C_ADDR 0x40
#define TRIGGER_TEMP_HOLD 0xE3
#define TRIGGER_HUMID_HOLD 0xE5

#define MEASUREMENT_MASK 0xFC
#define MEASUREMENT_TYPE_MASK 0x02
#define TEMEPARTURE_MEASUREMENT 0x00
#define HUMIDITY_MEASUREMENT 0x02

struct htu21d *htu21d_init(struct I2cBus *i2c_bus)
{
    if (!i2c_bus)
    {
        return NULL;
    }

    struct htu21d *ret = (struct htu21d *)malloc(sizeof(struct htu21d));

    if (!ret)
    {
        return NULL;
    }

    ret->i2c_bus = i2c_bus;

    return ret;
}

static struct htu21d_measurement get_measurement(struct htu21d *self, uint8_t command)
{
    struct htu21d_measurement res;
    uint8_t data[3];

    if (self == NULL || self->i2c_bus == NULL)
    {
        goto err_out;
    }

    if (i2c_read_register(self->i2c_bus, HTU21D_I2C_ADDR, command, data, 3) < 0)
    {
        goto err_out;
    }

    uint8_t computed_crc = compute_crc8(data, 2);

    if (computed_crc != data[2])
    {
        printf("Wrong CRC: computed:%d | received:%d", computed_crc, data[2]);
        goto err_out;
    }

    uint16_t raw = (data[0] << 8) | (data[1] & MEASUREMENT_MASK);

    if ((data[1] & MEASUREMENT_TYPE_MASK) == TEMEPARTURE_MEASUREMENT)
    {
        // Temperature measurement
        res.is_valid = true;
        res.value = -46.85 + (175.72 * raw) / 65536.0;
    }
    else
    {
        // Humidity measurement
        res.is_valid = true;
        res.value = -6.0 + (125.0 * raw) / 65536.0;
    }
    return res;

err_out:
    res.is_valid = false;
    return res;
}

struct htu21d_measurement htu21d_read_temperature(struct htu21d *self)
{
    return get_measurement(self, TRIGGER_TEMP_HOLD);
}

struct htu21d_measurement htu21d_read_humidity(struct htu21d *self)
{
    return get_measurement(self, TRIGGER_HUMID_HOLD);
}

void htu21d_close(struct htu21d *self)
{
    free(self);
}
