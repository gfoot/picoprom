#include "picoprom.h"

#include <stdio.h>
#include <tusb.h>

#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "configs.h"
#include "xmodem.h"
#include "eeprom.h"
#include "i2c.h"

static bool setup()
{
	bi_decl(bi_program_description("PicoPROM - EEPROM programming tool"));



	stdio_init_all();

	while (!tud_cdc_connected()) sleep_ms(100);
	printf("\nUSB Serial connected\n");





	init_settings();

	return true;
}


static void banner()
{
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");

	printf("PicoPROM v0.21   Raspberry Pi Pico DIP-EEPROM programmer\n");
	printf("                 by George Foot, February 2021\n");
	printf("                 https://github.com/gfoot/picoprom\n");
}



static bool input_handler(int c)
{
	if (c == 13)
	{
		change_settings();
		return true;
	}
	return false;
}


void loop()
{
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");

	banner();

	printf("\n\n");
	
	show_settings();
	
	printf("\n\n");
	printf("Ready to program - send data now, or press Enter to change settings\n");
	printf("\n");

	char buffer[65536];
	int sizeReceived = xmodem_receive(buffer, sizeof buffer, NULL, input_handler);


	bool eepromOk = false;
	if(gConfig.i2c){
		eepromOk = i2c_eeprom_init();
		if (!eepromOk)
		{
			printf("Device not connected.\n");

		}
	}else{
		eepromOk = eeprom_init();
		if (!eepromOk)
		{
			printf("ERROR: no pin was mapped to Write Enable (ID 15)\n");
		}

	}

	if (sizeReceived < 0)
	{
		xmodem_dumplog();
		printf("XMODEM transfer failed\n");
	}

	if (sizeReceived <= 0)
	{
		return;
	}

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

	if(gConfig.i2c){
		i2c_eeprom_writeImage(buffer, sizeReceived);
	}else{
		eeprom_writeImage(buffer, sizeReceived);
	}


	printf("\n");
	printf("Done\n");
}


int main()
{

	
	if (!setup())
		return -1;

	while (true)
		loop();
	
	return 0;
}

