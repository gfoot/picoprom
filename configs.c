#include "configs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmodem.h"


picoprom_config_t gConfig;


static picoprom_config_t gConfigs[] =
{
	{
		"AT28C256",
		32768,
		64, 10, 1, 0, 0,
		false, true, false
	},
	{
		"AT28C256F",
		32768,
		64, 3, 1, 0,
		false, true, false
	},
	{
		"AT28C64",
		8192,
		0, 10, 1, 1000, 0,
		false, false, false
	},
	{
		"AT28C64B",
		8192,
		64, 10, 1, 0, 0,
		false, true, false
	},
	{
		"AT28C64E",
		8192,
		0, 10, 1, 200, 0,
		false, false, false
	},
	{
		"AT28C16",
		2048,
		0, 10, 1, 1000, 0,
		false, false, false
	},
	{
		"AT28C16E",
		2048,
		0, 10, 1, 200, 0,
		false, false, false
	},
	{
		"M28C16",
		2048,
		64, 5, 1, 100, 0,
		false, true, false
	},
	{
		"AT24C04",
		1024,
		16, 3, 1, 0, 0x50,
		true, true, false
	},
	{
		NULL
	}
};


static int gConfigIndex = 0;


void init_settings()
{
	gConfigIndex = 0;
	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	xmodem_config.logLevel = 1;
}


void show_settings()
{
	printf("EEPROM Device: %s\n", gConfig.name);
	printf("        Capacity: %dK bytes\n", gConfig.size / 1024);
	printf("        Page mode: %s\n", gConfig.pageSize ? "on" : "off");
	if (gConfig.pageSize)
	{
		printf("        Page size: %d bytes\n", gConfig.pageSize);
		printf("        Page delay: %dms\n", gConfig.pageDelayMs);
	}
	printf("        Pulse delay: %dus\n", gConfig.pulseDelayUs);
	printf("        Byte delay: %dus\n", gConfig.byteDelayUs);
	printf("        Write protect: %s\n", gConfig.writeProtect ? "enable" : gConfig.writeProtectDisable ? "disable" : "no action / not supported");
	printf("\n");
	printf("Serial protocol: XMODEM+CRC\n");
	printf("        Block size: 128 bytes\n");
	printf("        CRC: on\n");
	printf("        Escaping: off\n");
	printf("        Log level: %d\n", xmodem_config.logLevel);
}


static void change_device()
{
	++gConfigIndex;
	if (!gConfigs[gConfigIndex].name)
		gConfigIndex = 0;

	memcpy(&gConfig, &gConfigs[gConfigIndex], sizeof gConfig);

	printf("\n\nChanged device to %s\n", gConfig.name);
}

static void change_log_level()
{
	xmodem_config.logLevel = (xmodem_config.logLevel + 1) % 4;

	printf("\n\nChanged log level to %d\n", xmodem_config.logLevel);
}


typedef struct
{
	char key;
	const char* commandName;
	void (*action)();
} command_t;

static command_t gCommands[] =
{
	{ 'd', "change device", change_device },
	{ 'l', "change log level", change_log_level },
	{ 'p', "return to programming mode", NULL },
	{ 0 }
};
uint8_t getKey(void) {     // hardware-independent wrapper
    uint8_t ch_read;
    do {
        ch_read = getchar_timeout_us(0); // 4 tries per 87 uSec char window at 115200 bps
    } while ((ch_read == '\0') || (ch_read == 0xff));
    return ch_read;
}

void change_settings()
{
	while (true)
	{
		printf("\n\n");
		printf("\n\n");
		printf("\n\n");

		show_settings();
		
		printf("\n\n");
		
		printf("Changing settings:\n");
		printf("\n");
		
		for (int i = 0; gCommands[i].key; ++i)
		{
			printf("    %c = %s\n", gCommands[i].key, gCommands[i].commandName);
		}
		printf("\n");
		printf("?");

		int c = getKey();
		for (int i = 0; gCommands[i].key; ++i)
		{
			if (c == gCommands[i].key)
			{
				if (!gCommands[i].action)
					return;

				gCommands[i].action();
			}
		}
	}
}

