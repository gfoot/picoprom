#ifndef INCLUDED_PICOPROM_H
#define INCLUDED_PICOPROM_H

#pragma once

#include <stdbool.h>


typedef struct
{
	const char *name;
	int size;
	int pageSize;
	int pageDelayMs;
	int pulseDelayUs;
	bool writeProtect;
	bool writeProtectDisable;
} picoprom_config_t;


#endif

