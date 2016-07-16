/*
 * ladder.c
 *
 *  Created on: 30/nov/2015
 *      Author: Codematica
 */
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "ld_stack.h"

uint8_t ld_stack[STACK_SIZE];
uint32_t ld_stack_pointer = 0;

bool ld_stack_ss() {
	ld_stack[ld_stack_pointer] = STACK_CMD_SS;
	ld_stack_pointer++;

	return true;
}
bool ld_stack_se() {
	ld_stack_pointer--;
	uint8_t branch_value = ld_stack[ld_stack_pointer];

	if (branch_value == STACK_CMD_SS) return true;

	while (1) {
		ld_stack_pointer--;
		uint8_t element_value = ld_stack[ld_stack_pointer];

		if ((element_value == STACK_CMD_SS) || (ld_stack_pointer == 0)) break;

		branch_value = branch_value && element_value;
	}

	ld_stack[ld_stack_pointer] = branch_value;
	ld_stack_pointer++;

	return true;
}

bool ld_stack_ps() {
	ld_stack[ld_stack_pointer] = STACK_CMD_PS;
	ld_stack_pointer++;

	return true;
}
bool ld_stack_pe() {
	ld_stack_pointer--;
	uint8_t branch_value = ld_stack[ld_stack_pointer];

	if (branch_value == STACK_CMD_PS) return true;

	while (1) {
		ld_stack_pointer--;
		uint8_t element_value = ld_stack[ld_stack_pointer];

		if ((element_value == STACK_CMD_PS) || (ld_stack_pointer == 0)) break;

		branch_value = (bool)branch_value || (bool)element_value;
	}

	ld_stack[ld_stack_pointer] = branch_value;
	ld_stack_pointer++;

	return true;
}

bool ld_stack_push(uint8_t value) {
	ld_stack[ld_stack_pointer] = value;
	ld_stack_pointer++;

	return true;
}

bool ld_stack_load(bool state) {
	ld_stack[ld_stack_pointer] = state ? STACK_CMD_1 : STACK_CMD_0;
	ld_stack_pointer++;

	return true;
}

void ld_stack_clear() {
	ld_stack_pointer = 0;
}

bool ld_stack_result() {
	volatile uint32_t i = 0;
	bool result = ld_stack[ld_stack_pointer - 1];
	bool isSeries = true;

	for (i = 1; i < ld_stack_pointer; i++) {
		uint8_t stack_value = ld_stack[ld_stack_pointer - i];

		if (stack_value == STACK_CMD_SS) isSeries = false;
		if (stack_value == STACK_CMD_PS) isSeries = true;

		if ((stack_value == STACK_CMD_0) || (stack_value == STACK_CMD_1)) {
			if (isSeries) {
				result &= (stack_value == STACK_CMD_1) ? true : false;
			}
		}
	}

	return result;
}

bool ld_stack_close() {
	while (1) {
		ld_stack_pointer--;
		uint8_t element_value = ld_stack[ld_stack_pointer];

		if ((element_value == STACK_CMD_PS) || (element_value == STACK_CMD_SS)) {
			ld_stack_pointer++;
			return true;
		}
	}

	return true;
}
