#ifndef INCLUDED_CONFIGS_H
#define INCLUDED_CONFIGS_H

#pragma once

#include <stdbool.h>


typedef struct
{
	const char *name;
	int size;
	int pageSize;
	int pageDelayMs;
	int pulseDelayUs;
	int byteDelayUs;
	int i2cAddress;
	bool i2c;
	bool writeProtect;
	bool writeProtectDisable;
} picoprom_config_t;


extern picoprom_config_t gConfig;


void init_settings();
void show_settings();
void change_settings();


#endif

