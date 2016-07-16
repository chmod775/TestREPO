/*
 * gram.c
 *
 *  Created on: 29/nov/2015
 *      Author: chmod775
 */
#include "board.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "gram.h"

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

/***** ADDRESSING *****/
/* 0xxxxxxx - GRAM    */
/* 8xxxxxxx - HW	  */
/**********************/

uint8_t gram[GRAM_SIZE]; // 20kb of RAM, yeah
uint8_t gramTYPE[GRAM_SIZE / 8 / 4]; // (0)Integer or (1)Float? That is the question

uint16_t gram_last_alloc = 0;
uint16_t gram_last_ralloc = 0;

uint16_t B_last_address = 0;
uint16_t V_last_address = 0;

void gram_reset() {
	B_last_address = 0;
	V_last_address = 0;
	gram_last_tagAddrItem = 0;
}

void gram_clear() {
	memset(gram, 0x00, GRAM_SIZE);
}

uint16_t gram_allocB() {
	uint16_t ret;
	if ((B_last_address & 0x0003) == 0) {
		B_last_address = MAX(B_last_address & 0xfffc, V_last_address);
	}
	ret = B_last_address;
	B_last_address++;
	return ret;
}

uint16_t gram_allocV() {
	V_last_address = MAX((B_last_address & 0xfffc) + ((B_last_address & 0x0003) > 0 ? 4 : 0), V_last_address);
	uint16_t ret = V_last_address;
	V_last_address += 4;
	return ret;
}

uint16_t gram_last_tagAddrItem = 0;
uint16_t *gram_tagAddrItem() {
	volatile uint16_t *t = (uint16_t *)&gram[gram_last_tagAddrItem];
	gram_last_tagAddrItem += 2;
	return t;
}

uint16_t gram_read_tagAddrItem(uint16_t index) {
	volatile uint16_t *t = (uint16_t *)&gram[index << 1];
	return *t;
}


// ##### WARNING: Sequential allocation, no space saving! ##### //

uint8_t *gram_alloc(uint16_t size) {
	uint8_t *ret = gram[gram_last_alloc];
	gram_last_alloc += size;
	return ret;
}

uint8_t *gram_ralloc(uint16_t size) { // Reverse alloc, from bottom to top. Like a stack (used for growing arrays)
	uint8_t *ret = gram[gram_last_alloc];
	gram_last_alloc += size;
	return ret;
}

uint8_t gram_read_byte(uint16_t address) { // Unused ?
	uint8_t ret = 0;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ret = gram[address & 0x7fff];
	}

	return ret;
}


INLINE bool gram_gettype(uint16_t address) {
	uint32_t t = address >> 2;
	return gramTYPE[t >> 3] & (1 << (t & 0x07));
}

INLINE void gram_settype(uint16_t address, uint8_t type) {
	volatile uint32_t t = address >> 2;
	gramTYPE[t >> 3] |= (type << (t & 0x07));
}

INLINE void gram_settype_float(uint16_t address) {
	volatile uint32_t t = address >> 2;
	gramTYPE[t >> 3] |= (1 << (t & 0x07));
}
INLINE void gram_settype_int(uint16_t address) {
	volatile uint32_t t = address >> 2;
	gramTYPE[t >> 3] &= ~(1 << (t & 0x07));
}




uint16_t gram_temp_pointer = 0; // From bottom to top

uint16_t gram_temp_value() {
	gram_temp_pointer += 4;
	uint16_t ret = GRAM_SIZE - gram_temp_pointer - 100;
	return ret;
}
uint16_t gram_temp_byte() {
	gram_temp_pointer++;
	uint16_t ret = GRAM_SIZE - gram_temp_pointer - 100;
	return ret;
}


bool gram_read_bool(uint16_t address) {
	bool ret = false;
	if (address & 0x8000) { // Hardware addressing
		ret = hw_read_bool(address & 0x7fff) ? true : false;
	} else { // GRAM addressing
		ret = gram[address & 0x7fff] ? true : false;
	}

	return ret;
}

uint32_t gram_read_value(uint16_t address) {
	uint32_t ret;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ret = *((uint32_t *)(&gram[address & 0x7fff]));
	}

	return ret;
}

uint32_t gram_read_int(uint16_t address) {
	uint32_t ret;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ret = *((uint32_t *)(&gram[address & 0x7fff]));
	}

	return ret;
}

float gram_read_float(uint16_t address) {
	float ret;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ret = *((float *)(&gram[address & 0x7fff]));
	}

	return ret;
}

uint32_t gram_read_int_forced(uint16_t address) {
	uint32_t ret;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		if (gram_gettype(address))
			ret = (uint32_t)(*((float *)(&gram[address & 0x7fff])));
		else
			ret = *((uint32_t *)(&gram[address & 0x7fff]));
	}

	return ret;
}

float gram_read_float_forced(uint16_t address) {
	float ret;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		if (gram_gettype(address))
			ret = *((float *)(&gram[address & 0x7fff]));
		else
			ret = (float)(*((uint32_t *)(&gram[address & 0x7fff])));
	}

	return ret;
}


void gram_write_bool(uint16_t address, bool state) {
	if (address & 0x8000) { // Hardware addressing
		hw_write_bool(address & 0x7fff, state);
	} else { // GRAM addressing
		gram[address & 0x7fff] = state ? 0xff : 0x00;
	}
}

void gram_write_value(uint16_t address, uint32_t value) {
	uint32_t *ptr;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ptr = &gram[address & 0x7fff];
		*ptr = value;
	}
}

void gram_write_int(uint16_t address, uint32_t value) {
	uint32_t *ptr;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ptr = (uint32_t *)(&gram[address & 0x7fff]);
		*ptr = value;

		gram_settype_int(address);
	}
}
void gram_write_float(uint16_t address, float value) {
	float *ptr;
	if (address & 0x8000) { // Hardware addressing

	} else { // GRAM addressing
		ptr = (float *)(&gram[address & 0x7fff]);
		*ptr = value;

		gram_settype_float(address);
	}
}
