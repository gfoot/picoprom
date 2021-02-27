#include "configs.h"

#include <stdlib.h>


picoprom_config_t gConfigs[] =
{
	{
		"AT28C256",
		32768,
		64, 10, 1,
		true, false
	},
	{
		"AT28C64B",
		8192,
		64, 10, 1,
		true, false
	},
	{
		"AT28C64",
		8192,
		0, 10, 1,
		false, false
	},
	{
		"AT28C16",
		2048,
		0, 10, 1,
		false, false
	},
	{
		NULL
	}
};

