#ifndef INCLUDED_I2C_H
#define INCLUDED_I2C_H

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

bool i2c_eeprom_init();

void i2c_eeprom_writeImage(const uint8_t* buffer, size_t size);
int i2c_eeprom_readImage(uint8_t* buffer, size_t size);

static int i2c_readByte(uint8_t word_address, uint8_t *data);
static int i2c_writeByte(uint8_t word_address, uint8_t *data);

#endif
