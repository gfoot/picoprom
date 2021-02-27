#include "picoprom.h"

#include <stdio.h>
#include <tusb.h>

#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "configs.h"
#include "xmodem.h"


static picoprom_config_t gConfig;



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
const int gPinMapping[] =
{
	-1, -1,  13, 14, 12, 7,  6, 5, 4, 3,  2, 1, 0, 16,  17, 18,

	19, 20,  21, 22, 23, 15,  10,  -1, -1, -1,  11, 9,  8,
};

const int gLedPin = 25;

int gWriteEnablePin;

/* Mask of all GPIO bits used for EEPROM pins */
uint32_t gAllBits;


/* Pack address, data, and writeEnable into a GPIO word */
uint32_t packGpioBits(uint16_t address, uint8_t data, bool writeEnable)
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


void writeByte(uint16_t address, uint8_t data)
{
	uint32_t bits = packGpioBits(address, data, false);

	gpio_put_masked(gAllBits, bits | (1 << gWriteEnablePin));
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits);
	busy_wait_us(gConfig.pulseDelayUs);
	gpio_put_masked(gAllBits, bits | (1 << gWriteEnablePin));
}


void writeImage(const uint8_t* buffer, size_t size)
{
	if (gConfig.writeProtectDisable)
	{
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x80);
		writeByte(0x5555, 0xaa);
		writeByte(0x2aaa, 0x55);
		writeByte(0x5555, 0x20);
		sleep_ms(10);
	}

	if (size > gConfig.size) size = gConfig.size;

	int prevAddress = -1;
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


int main()
{
	bi_decl(bi_program_description("PicoPROM - EEPROM programming tool"));

	gAllBits = packGpioBits(0xffff, 0xff, true);

	gpio_init(gLedPin);
	gpio_set_dir(gLedPin, GPIO_OUT);
	gpio_put(gLedPin, 1);

	gpio_init_mask(gAllBits);
	gpio_set_dir_out_masked(gAllBits);
	gpio_put_masked(gAllBits, gAllBits);


	stdio_init_all();

	while (!tud_cdc_connected()) sleep_ms(100);
	printf("\nUSB Serial connected\n");


	gWriteEnablePin = -1;
	for (int i = 0; i < sizeof gPinMapping / sizeof *gPinMapping; ++i)
	{
		if (gPinMapping[i] == 15)
			gWriteEnablePin = i;
	}
	if (gWriteEnablePin == -1)
	{
		printf("ERROR: no pin was mapped to Write Enable (ID 15)\n");
		return -1;
	}


	memcpy(&gConfig, &gConfigs[0], sizeof gConfig);

	xmodem_config.logLevel = 1;


	while (true)
	{
		printf("\n\n");
		printf("PicoPROM v0.1 - Raspberry Pi Pico DIP-EEPROM programmer\n");
		printf("                by George Foot, February 2021\n");
		printf("                https://github.com/gfoot/picoprom\n");
		printf("\n\n");
		printf("EEPROM Device: AT28C256\n");
		printf("        Capacity: %dK bytes\n", gConfig.size / 1024);
		printf("        Page mode: %s\n", gConfig.pageSize ? "on" : "off");
		if (gConfig.pageSize)
		{
			printf("        Page size: %d bytes\n", gConfig.pageSize);
			printf("        Page delay: %dms\n", gConfig.pageDelayMs);
		}
		printf("        Pulse delay: %dus\n", gConfig.pulseDelayUs);
		printf("        Write protect: %s\n", gConfig.writeProtect ? "enable" : gConfig.writeProtectDisable ? "disable" : "no action / not supported");
		printf("\n");
		printf("Serial protocol: XMODEM+CRC\n");
		printf("        Block size: 128 bytes\n");
		printf("        CRC: on\n");
		printf("        Escaping: off\n");
		printf("        Log level: %d\n", xmodem_config.logLevel);
		printf("\n\n");
		printf("Ready to program - send data now\n");
		printf("\n");

		char buffer[65536];
		int sizeReceived = xmodem_receive(buffer, sizeof buffer, NULL);

		if (sizeReceived >= 0)
		{
			printf("\n");
			printf("Transfer complete - received %d bytes\n", sizeReceived);
			printf("\n");

			if (sizeReceived > gConfig.size)
			{
				printf("Truncating image to %d bytes\n", gConfig.size);
				printf("\n");
				sizeReceived = gConfig.size;
			}

			printf("Writing to EEPROM...\n");
			writeImage(buffer, sizeReceived);

			printf("\n");
			printf("Done\n");
		}
		else
		{
			xmodem_dumplog();
			printf("XMODEM transfer failed\n");
		}
	}

	return 0;
}

