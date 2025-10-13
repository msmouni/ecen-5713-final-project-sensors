#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdio.h>
#include <stdint.h>

struct I2cBus
{
    int i2c_fd;
};

// Function to initialize I2C
struct I2cBus *i2c_init(char *i2c_path);

// Function to read a register from a given device
int i2c_read_register(struct I2cBus *self, uint8_t device_addr, uint8_t reg, uint8_t *buffer, size_t len);

// Function to write a register from a given device
int i2c_write_register(struct I2cBus *self, uint8_t device_addr, uint8_t reg, uint8_t val);

int i2c_close(struct I2cBus *self);

#endif /* I2C_BUS_H */
