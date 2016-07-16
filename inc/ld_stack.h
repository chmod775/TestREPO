/*
 * ld_stack.h
 *
 *  Created on: 30/nov/2015
 *      Author: Codematica
 */

#ifndef INC_LD_STACK_H_
#define INC_LD_STACK_H_


/* STACK COMMANDS */
#define STACK_CMD_0		0x00	// Value @ 0
#define STACK_CMD_1		0xFF	// Value @ 1

#define STACK_CMD_SS	0x10	// Series start
#define STACK_CMD_SE	0x11	// Series end

#define STACK_CMD_PS	0x20	// Parallel start
#define STACK_CMD_PE	0x21	// Parallel end


#define STACK_SIZE	0x200
extern uint8_t ld_stack[];
extern uint32_t ld_stack_pointer;

bool ld_stack_ss();
bool ld_stack_se();

bool ld_stack_ps();
bool ld_stack_pe();

bool ld_stack_push(uint8_t value);
bool ld_stack_load(bool state);

void ld_stack_clear();

bool ld_stack_result();
bool ld_stack_close();

#endif /* INC_LD_STACK_H_ */
