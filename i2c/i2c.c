#include "i2c.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <malloc.h>

// Function to initialize I2C
struct I2cBus *i2c_init(char *i2c_path)
{
    int i2c_fd = open(i2c_path, O_RDWR);
    if (i2c_fd < 0)
    {
        perror("Failed to open I2C bus");
        return NULL;
    }

    struct I2cBus *ret = (struct I2cBus *)malloc(sizeof(struct I2cBus));

    if (!ret)
    {
        return ret;
    }

    ret->i2c_fd = i2c_fd;

    return ret;
}

// Function to read a register from a given device
int i2c_read_register(struct I2cBus *self, uint8_t device_addr, uint8_t reg, uint8_t *buffer, size_t len)
{
    if (!self)
    {
        return -1;
    }

    if (ioctl(self->i2c_fd, I2C_SLAVE, device_addr) < 0)
    {
        perror("Failed to select I2C device");
        return -1;
    }

    // Write register address
    if (write(self->i2c_fd, &reg, 1) != 1)
    {
        perror("Failed to write register address");
        return -1;
    }

    // Read data from register
    if (read(self->i2c_fd, buffer, len) != (ssize_t)len)
    {
        perror("Failed to read data");
        return -1;
    }

    return 0;
}

int i2c_write_register(struct I2cBus *self, uint8_t device_addr, uint8_t reg, uint8_t value)
{
    if (!self)
    {
        return -1;
    }

    if (ioctl(self->i2c_fd, I2C_SLAVE, device_addr) < 0)
    {
        perror("Failed to select I2C device");
        return -1;
    }

    uint8_t config[2] = {reg, value};

    if (write(self->i2c_fd, config, 2) != 2)
    {
        perror("Failed to write register address");
        return -1;
    }

    return 0;
}

int i2c_close(struct I2cBus *self)
{
    if (!self)
    {
        return -1;
    }

    int ret = close(self->i2c_fd);

    free(self);

    return ret;
}
