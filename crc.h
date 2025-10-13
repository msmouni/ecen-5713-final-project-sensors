#ifndef HTU21D_CRC_H
#define HTU21D_CRC_H

#include <stdint.h>

// Function to compute CRC-8 using polynomial 0x31 (x⁸ + x⁵ + x⁴ + 1)
uint8_t compute_crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0x00; // Initial value

    for (uint8_t i = 0; i < length; i++)
    {
        crc ^= data[i]; // XOR with byte

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31; // Polynomial 0x31
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#endif /* HTU21D_CRC_H */
