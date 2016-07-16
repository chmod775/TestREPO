/*
 * debug.c
 *
 *  Created on: 10/gen/2016
 *      Author: chmod775
 */

#include "board.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cdc_vcom.h"
#include "debug.h"

uint32_t debug_msg_cnt = 0;

char hexChar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void DEBUG_MSG(char *str) {
	char msg_raw[256];

	memset(msg_raw, 0, 256);
	sprintf(msg_raw, "DEBUG #%d: %s\r\n", debug_msg_cnt, str);
	debug_msg_cnt++;

	vcom_write(msg_raw, strlen(msg_raw));
}

void DEBUG_PRINT(char *fmt, ...) {
	va_list arg;
	char msg_raw[256];

	memset(msg_raw, 0, 256);
	va_start(arg, fmt);
	vsprintf(msg_raw, fmt, arg);
	va_end(arg);

	debug_msg_cnt++;

	vcom_write(msg_raw, strlen(msg_raw));
}

void DEBUG_HEX(uint8_t *array, unsigned int n) {
	char msg_raw[256];
	unsigned int cnt = 0;

	int row, col, charPos = 0;

	for (row = 0; row < n / 16; row++) {

		memset(msg_raw, 0, 256);
		sprintf(msg_raw, "$%04X  ", row * 16);
		charPos = 7;

		for (col = 0; col < 16; col++) {
			msg_raw[charPos] = hexChar[array[cnt] >> 4];
			msg_raw[charPos + 1] = hexChar[array[cnt] & 0x0f];
			msg_raw[charPos + 2] = ' ';
			charPos += 3;

			cnt++;
			if (cnt > n) break;
		}

		msg_raw[charPos] = '\r';
		msg_raw[charPos + 1] = '\n';

		vcom_write(msg_raw, strlen(msg_raw));

		if (cnt > n) break;
	}
}
