#ifndef INCLUDED_XMODEM_H
#define INCLUDED_XMODEM_H

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* Protocol variants */
typedef enum
{
	XMODEM_MODE_ORIGINAL,
	XMODEM_MODE_CRC
} xmodem_mode_t;


/* Configuration parameters */
typedef struct
{
	/* Enable/disable logging during the transfer.
	 *
	 * 0 = fatal errors only
	 * 1 = errors only
	 * 2 = verbose logging
	 * 3 = log all bytes received
	 */
	int logLevel;

	/* Enable the CRC variant of the protocol */
	bool useCrc;

	/* Enable decoding of escape characters */
	bool useEscape;
} xmodem_config_t;


/* Currently active configuration */
extern xmodem_config_t xmodem_config;


/* Change configuration */
void xmodem_set_config(xmodem_mode_t mode);


/* Receive a file.  Note that XMODEM pads files to the next multiple of 128, 
 * using character 26.
 *
 * Returns the amount of data received - always a multiple of 128.
 *
 *    outputBuffer: where to store the received data
 *    bufferSize:   maximum number of bytes to store in outputBuffer
 *    message:      an optional prompt to start the transfer
 *
 * If message is provided, it will be printed every few seconds until the 
 * first data is received.
 */
int xmodem_receive(void* outputBuffer, size_t bufferSize, const char *message);


/* Dumps cached log data to stdout. */
void xmodem_dumplog();


#endif

