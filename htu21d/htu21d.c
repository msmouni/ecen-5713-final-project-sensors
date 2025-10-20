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

// Non-hold master commands
#define TRIGGER_TEMP_NO_HOLD 0xF3
#define TRIGGER_HUMID_NO_HOLD 0xF5

#define MEASUREMENT_MASK 0xFC
#define MEASUREMENT_TYPE_MASK 0x02
#define TEMPERATURE_MEASUREMENT 0x00
#define HUMIDITY_MEASUREMENT 0x02

#define HTU21D_MEAS_DELAY_US 100000 // 100ms max measurement time

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

/****************** Hold master commands ******************/
static struct htu21d_measurement get_measurement_hold(struct htu21d *self, uint8_t command)
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

    if ((data[1] & MEASUREMENT_TYPE_MASK) == TEMPERATURE_MEASUREMENT)
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

struct htu21d_measurement htu21d_read_temperature_hold(struct htu21d *self)
{
    return get_measurement_hold(self, TRIGGER_TEMP_HOLD);
}

struct htu21d_measurement htu21d_read_humidity_hold(struct htu21d *self)
{
    return get_measurement_hold(self, TRIGGER_HUMID_HOLD);
}

/****************** Non-hold master commands ******************/
static struct htu21d_measurement get_measurement_no_hold(struct htu21d *self, uint8_t command, uint8_t meas_type)
{
    struct htu21d_measurement res = {.is_valid = false, .value = 0};
    uint8_t data[3];

    if (!self || !self->i2c_bus)
        return res;

    /* trigger measurement */
    if (i2c_write(self->i2c_bus, HTU21D_I2C_ADDR, &command, 1) < 0)
    {
        perror("HTU21D: failed to trigger measurement");
        return res;
    }

    /* wait for conversion to finish */
    usleep(HTU21D_MEAS_DELAY_US);

    /* read measurement */
    if (i2c_read(self->i2c_bus, HTU21D_I2C_ADDR, data, 3) < 0)
    {
        perror("HTU21D: failed to read data");
        return res;
    }

    /* verify CRC */
    uint8_t computed_crc = compute_crc8(data, 2);
    if (computed_crc != data[2])
    {
        fprintf(stderr, "HTU21D: CRC mismatch (calc=%d, got=%d)\n", computed_crc, data[2]);
        return res;
    }

    /* convert raw value */
    uint16_t raw = (data[0] << 8) | (data[1] & MEASUREMENT_MASK);

    if (meas_type == TEMPERATURE_MEASUREMENT)
        res.value = -46.85 + (175.72 * raw) / 65536.0;
    else
        res.value = -6.0 + (125.0 * raw) / 65536.0;

    res.is_valid = true;
    return res;
}

struct htu21d_measurement htu21d_read_temperature_no_hold(struct htu21d *self)
{
    return get_measurement_no_hold(self, TRIGGER_TEMP_NO_HOLD, TEMPERATURE_MEASUREMENT);
}

struct htu21d_measurement htu21d_read_humidity_no_hold(struct htu21d *self)
{
    return get_measurement_no_hold(self, TRIGGER_HUMID_NO_HOLD, HUMIDITY_MEASUREMENT);
}

void htu21d_close(struct htu21d *self)
{
    free(self);
}
