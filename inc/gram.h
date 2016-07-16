/*
 * gram.h
 *
 *  Created on: 02/dic/2015
 *      Author: Codematica
 */

#ifndef INC_GRAM_H_
#define INC_GRAM_H_

#define GRAM_SIZE	0x3000

extern uint8_t gram[]; // 20kb of RAM, yeah
extern uint8_t gramTYPE[]; // (1)Integer or (0)Float? That is the question

void gram_reset();
void gram_clear();

uint16_t gram_allocB();
uint16_t gram_allocV();

extern uint16_t gram_last_tagAddrItem;
uint16_t *gram_tagAddrItem();
uint16_t gram_read_tagAddrItem(uint16_t index);

INLINE bool gram_gettype(uint16_t address);
INLINE void gram_settype(uint16_t address, uint8_t type);
INLINE void gram_settype_float(uint16_t address);
INLINE void gram_settype_int(uint16_t address);

uint8_t *gram_alloc(uint16_t size);
uint8_t *gram_ralloc(uint16_t size);

uint16_t gram_temp_value();
uint16_t gram_temp_byte();


/* ##### READ ##### */
uint8_t gram_read_byte(uint16_t address);

bool gram_read_bool(uint16_t address);
uint32_t gram_read_value(uint16_t address);

uint32_t gram_read_int(uint16_t address);
float gram_read_float(uint16_t address);

uint32_t gram_read_int_forced(uint16_t address);
float gram_read_float_forced(uint16_t address);


/* ##### WRITE ##### */
void gram_write_bool(uint16_t address, bool state);
void gram_write_value(uint16_t address, uint32_t value);

void gram_write_int(uint16_t address, uint32_t value);
void gram_write_float(uint16_t address, float value);

#endif /* INC_GRAM_H_ */
