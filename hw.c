/*
 * hw.c
 *
 *  Created on: 02/dic/2015
 *      Author: Codematica
 */

#include "board.h"
#include <stdio.h>
#include "gpio.h"
#include "gram.h"

/***** HARDWARE ADDRESSING *****/
/* 00000000 - GPIO state       */
/* 00000100 - GPIO direction   */
/* 							   */
/*******************************/

bool hw_read_bool(uint16_t address) {
	uint16_t t_address = address & 0x7fff;
	if (t_address < 0x100) return GPIO_state_read(t_address);

	return false;
}

void hw_write_bool(uint16_t address, bool state) {
	uint16_t t_address = address & 0x7fff;
	if (t_address < 0x100) GPIO_state(t_address, state);
}

uint16_t hw_marsh_input_table[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
uint16_t hw_marsh_output_table[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

void hw_marshalling_input() {
	volatile uint32_t i;
	for (i = 0; i < 16; i++) {
		if (hw_marsh_input_table[i] != -1) {
			gram_write_bool(hw_marsh_input_table[i], GPIO_state_read(i));
		}
	}
}

void hw_marshalling_output() {
	volatile uint32_t i;
	for (i = 0; i < 16; i++) {
		if (hw_marsh_output_table[i] != -1) {
			GPIO_state(i, gram_read_bool(hw_marsh_output_table[i]));
		}
	}
}
