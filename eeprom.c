#include "eeprom.h"

#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "configs.h"


/* Configure which EEPROM pin is connected to each GPIO pin.
 *
 * EEPROM pins are referenced here as follows:
 *    A0-A14      0-14
 *    WE          15
 *    I/O0-I/O7   16-23
 *
 * VCC, GND, CE and OE are not driven and should be permanently connected
 * to the supply rails (OE to VCC, CE to GND).
 *
 * Negative numbers indicate none of the above are connected to this GPIO pin.
 */
static const int gPinMapping[] =
{
	-1, -1,  13, 14, 12, 7,  6, 5, 4, 3,  2, 1, 0, 16,  17, 18,

	19, 20,  21, 22, 23, 15,  10,  -1, -1, -1,  11, 9,  8,
};

static const int gLedPin = 25;

int gWriteEnablePin;

/* Mask of all GPIO bits used for EEPROM pins */
static uint32_t gAllBits;


/* Pack address, data, and writeEnable into a GPIO word */
static uint32_t packGpioBits(uint16_t address, uint8_t data, bool writeEnable)
{
	uint32_t tempPacking = address | ((uint32_t)data << 16) | (!!writeEnable << 15);

	uint32_t realPacking = 0;
	for (int i = 0; i < sizeof gPinMapping / sizeof *gPinMapping; ++i)
	{
		if (gPinMapping[i] >= 0 && tempPacking & (1 << gPinMapping[i]))
			realPacking |= 1 << i;
	}

	return realPacking;
}


static void writeByte(uint16_t address, uint8_t data)
{
	uint32_t bits = packGpioBits(address, data, false);

	gpio_put_masked(gAllBits, bits | (1 << gWriteEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits);
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits | (1 << gWriteEnablePin));
	busy_wait_us(gConfig.byteDelayUs);
}


void eeprom_writeImage(const uint8_t* buffer, size_t size)
{
	if (gConfig.writeProtectDisable)
	{
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x80);
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x20);
		sleep_ms(gConfig.pageDelayMs);
	}

	if (size > gConfig.size) size = gConfig.size;

	int prevAddress = 0xffffff;
	for (int address = 0; address < size; ++address)
	{
		if (gConfig.pageSize && prevAddress / gConfig.pageSize != address / gConfig.pageSize)
		{
			/* Page change - wait 10ms (worst case) for write to complete */
			sleep_ms(gConfig.pageDelayMs);

			if (gConfig.writeProtect)
			{
				/* Locking prefix */
				writeByte(0x5555, 0xaa);
				writeByte(0x2aaa, 0x55);
				writeByte(0x5555, 0xa0);
			}
		}

		prevAddress = address;
		
		writeByte(address, buffer[address]);

		gpio_put(gLedPin, (address & 0x100) != 0);

		if (((address + 1) & 0x7ff) == 0)
		{
			printf(" %dK", (address + 1) >> 10);
		}
	}
}


bool eeprom_init()
{
	gAllBits = packGpioBits(0xffff, 0xff, true);

	gpio_init(gLedPin);
	gpio_set_dir(gLedPin, GPIO_OUT);
	gpio_put(gLedPin, 1);

	gpio_init_mask(gAllBits);
	gpio_set_dir_out_masked(gAllBits);
	gpio_put_masked(gAllBits, gAllBits);

	gWriteEnablePin = -1;
	for (int i = 0; i < sizeof gPinMapping / sizeof *gPinMapping; ++i)
	{
		if (gPinMapping[i] == 15)
			gWriteEnablePin = i;
	}

	return gWriteEnablePin >= 0;
}

