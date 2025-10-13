#include "htu21d.h"
#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define HTU21D_I2C_ADDR 0x40
#define TRIGGER_TEMP_HOLD 0xE3
#define TRIGGER_HUMID_HOLD 0xE5

#define MEASUREMENT_MASK 0xFC
#define MEASUREMENT_TYPE_MASK 0x02
#define TEMEPARTURE_MEASUREMENT 0x00
#define HUMIDITY_MEASUREMENT 0x02

#define HTU21_I2C_DEV_PATH "/dev/i2c-1"

static struct htu21d_measurement get_measurement(struct htu21d *self)
{
    struct htu21d_measurement res;
    uint8_t data[3];

    if (read(self->i2c_fd, data, 3) != 3)
    {
        perror("Failed to read measurement");
        res.is_valid = false;
        goto out;
    }

    uint8_t computed_crc = compute_crc8(data, 2);

    if (computed_crc != data[2])
    {
        printf("Wrong CRC: computed:%d | received:%d", computed_crc, data[2]);
        res.is_valid = false;
        goto out;
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

out:
    return res;
}

struct htu21d *htu21d_init()
{
    struct htu21d *dev = calloc(1, sizeof(struct htu21d));

    if ((dev->i2c_fd = open(HTU21_I2C_DEV_PATH, O_RDWR)) < 0)
    {
        perror("Failed to open I2C device");
        return NULL;
    }
    if (ioctl(dev->i2c_fd, I2C_SLAVE, HTU21D_I2C_ADDR) < 0)
    {
        perror("Failed to set I2C address");
        return NULL;
    }
    return dev;
}

struct htu21d_measurement read_temperature(struct htu21d *self)
{
    uint8_t command = TRIGGER_TEMP_HOLD;

    write(self->i2c_fd, &command, 1);
    usleep(50000); // Wait for measurement

    return get_measurement(self);
}

struct htu21d_measurement read_humidity(struct htu21d *self)
{
    uint8_t command = TRIGGER_HUMID_HOLD;

    write(self->i2c_fd, &command, 1);
    usleep(50000); // Wait for measurement

    return get_measurement(self);
}

void htu21d_close(struct htu21d *self)

{
    close(self->i2c_fd);
}
