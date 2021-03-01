/* Basic XMODEM implementation for Raspberry Pi Pico
 *
 * This only implements the old-school checksum, not CRC, 
 * and only supports 128-byte blocks. But the whole point
 * of XMODEM was to be simple, and this works fine for ~32K
 * transfers over USB-serial.
 */


#include "xmodem.h"

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"


const int XMODEM_SOH = 1;
const int XMODEM_EOT = 4;
const int XMODEM_ACK = 6;
const int XMODEM_DLE = 0x10;
const int XMODEM_NAK = 0x15;
const int XMODEM_CAN = 0x18;

const int XMODEM_BLOCKSIZE = 128;


xmodem_config_t xmodem_config =
{
	1,    /* logLevel */
	true, /* useCrc */
	false /* useEscape */
};


static char gLogBuffer[65536];
static int gLogPos = 0;


static void xmodem_log(char *s)
{
	if (gLogPos + strlen(s) + 2 >= sizeof gLogBuffer)
	{
		gLogPos = sizeof gLogBuffer;
		return;
	}
	strcpy(gLogBuffer + gLogPos, s);
	gLogPos += strlen(s);
	gLogBuffer[gLogPos++] = '\n';
	gLogBuffer[gLogPos] = 0;
}

void xmodem_dumplog()
{
	if (gLogPos)
	{
		puts(gLogBuffer);
	}
}

static void xmodem_clearlog()
{
	gLogPos = 0;
	gLogBuffer[gLogPos] = 0;
}


void xmodem_set_config(xmodem_mode_t mode)
{
	bzero(&xmodem_config, sizeof xmodem_config);

	switch (mode)
	{
		case XMODEM_MODE_ORIGINAL:
			xmodem_config.useEscape = false;
			xmodem_config.useCrc = false;
			break;

		case XMODEM_MODE_CRC:
			xmodem_config.useEscape = false;
			xmodem_config.useCrc = true;
			break;
	}
}


int xmodem_receive(void* outputBuffer, size_t bufferSize, const char* message, bool (*inputhandler)())
{
	char logBuffer[1024];
	xmodem_clearlog();

	int sizeReceived = 0;
	int packetNumber = 1;

	bool eof = false;
	bool can = false;
	bool error = false;

	/* Receive a file */
	while (true)
	{
		absolute_time_t nextPrintTime = get_absolute_time();

		/* Receive next packet */
		while (true)
		{
			if (sizeReceived == 0 && get_absolute_time() > nextPrintTime)
			{
				xmodem_dumplog();

				if (message) puts(message);
				
				if (xmodem_config.useCrc)
				{
					putchar(8);
					putchar('C');
				}
				else
				{
					putchar(XMODEM_NAK);
				}

				nextPrintTime = make_timeout_time_ms(3000);
			}

			int c = getchar_timeout_us(1000);
			if (c == PICO_ERROR_TIMEOUT) continue;

			if (c == XMODEM_EOT || c == XMODEM_SOH || c == XMODEM_CAN)
			{
				eof = (c == XMODEM_EOT);
				can = (c == XMODEM_CAN);
				break;
			}
			else if (inputhandler && inputhandler(c))
			{
				return -1;
			}
			else if (xmodem_config.logLevel >= 1)
			{
				sprintf(logBuffer, "Unexected character %d received - expected SOH or EOT", c);
				xmodem_log(logBuffer);
			}
		}

		if (eof) 
		{
			if (xmodem_config.logLevel >= 2) xmodem_log("EOT => ACK");
			putchar(XMODEM_ACK);
			break;
		}

		if (can)
		{
			if (xmodem_config.logLevel >= 1) xmodem_log("CAN => ACK");
			putchar(XMODEM_ACK);
			break;
		}


		if (xmodem_config.logLevel >= 2)
		{
			sprintf(logBuffer, "Got SOH for packet %d", packetNumber);
			xmodem_log(logBuffer);
		}

		if (sizeReceived + XMODEM_BLOCKSIZE > bufferSize)
		{
			error = true;
			xmodem_log("Output buffer full");
			for (int i = 0; i < 8; ++i)
				putchar(XMODEM_CAN);
			while (getchar_timeout_us(1000) != PICO_ERROR_TIMEOUT);
			break;
		}


		bool timeout = false;
		absolute_time_t timeoutTime = make_timeout_time_ms(1000);

		int checksum = 0;
		bool escape = false;

		char buffer[2+XMODEM_BLOCKSIZE+2];
		int bufpos = 0;
		while (bufpos < 2+XMODEM_BLOCKSIZE + (xmodem_config.useCrc ? 2 : 1) && !timeout)
		{
			if (get_absolute_time() > timeoutTime)
			{
				if (xmodem_config.logLevel >= 1) xmodem_log("Timeout");
				timeout = true;
				break;
			}

			int c = getchar_timeout_us(1000);
			if (c == PICO_ERROR_TIMEOUT) continue;

			
			if (xmodem_config.logLevel >= 3)
			{
				sprintf(logBuffer, "Got %d", c);
				xmodem_log(logBuffer);
			}

			bool isData = (bufpos >= 2) && (bufpos < 2+XMODEM_BLOCKSIZE);

			if (xmodem_config.useEscape && isData && c == XMODEM_DLE)
			{
				escape = true;
				continue;
			}

			if (escape) c ^= 0x40;
			escape = false;

			buffer[bufpos++] = c;

			if (isData)
			{
				if (xmodem_config.useCrc)
				{
					checksum = checksum ^ (int)c << 8;
					for (int i = 0; i < 8; ++i)
						if (checksum & 0x8000)
							checksum = checksum << 1 ^ 0x1021;
						else
							checksum = checksum << 1;
				}
				else
				{
					checksum += c;
				}
			}
		}

		bool wrongPacket = (buffer[0] != (char)packetNumber);
		bool badPacketInv = (buffer[1] != (char)(255-buffer[0]));
		bool badChecksum = (buffer[2+XMODEM_BLOCKSIZE] != (char)checksum);
		if (xmodem_config.useCrc)
		{
			badChecksum = (buffer[2+XMODEM_BLOCKSIZE] != (char)(checksum>>8))
				|| (buffer[2+XMODEM_BLOCKSIZE+1] != (char)checksum);
		}

		if (timeout || wrongPacket || badPacketInv || badChecksum)
		{
			if (xmodem_config.logLevel >= 1) xmodem_log("NAK");
			putchar(XMODEM_NAK);
			continue;
		}

		if (xmodem_config.logLevel >= 2) xmodem_log("ACK");
		putchar(XMODEM_ACK);

		memcpy(outputBuffer+sizeReceived, buffer+2, XMODEM_BLOCKSIZE);

		sizeReceived += XMODEM_BLOCKSIZE;
		packetNumber++;
	}

	puts("");
	xmodem_dumplog();
	xmodem_clearlog();

	if (can || error) return -1;

	return sizeReceived;
}

