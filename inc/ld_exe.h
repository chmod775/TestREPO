/*
 * ld_exe.h
 *
 *  Created on: 03/dic/2015
 *      Author: chmod775
 */

#ifndef INC_LD_EXE_H_
#define INC_LD_EXE_H_

extern uint8_t ld_buffer[];

#define LD_CMD_ENDPROG	0x00
#define LD_CMD_NEWRUNG	0x08 // Clear the stack

#define LD_CMD_SS		0x01
#define LD_CMD_SE		0x02
#define LD_CMD_PS		0x03
#define LD_CMD_PE		0x04

#define LD_CMD_DBG		0x0F

#define LD_CMD_VLOAD	0xFE // Load a value (4 bytes)
#define LD_CMD_BLOAD	0xFF // Load a single byte

#define LD_CMD_XIC		0x10
#define LD_CMD_XIO		0x11

#define LD_CMD_OTE		0x20
#define LD_CMD_OTL		0x21
#define LD_CMD_OTU		0x22

#define LD_CMD_TON		0x30
#define LD_CMD_TOF		0x31

#define LD_CMD_OSR		0x40
#define LD_CMD_OSF		0x41

#define LD_CMD_SRV		0x50

#define LD_CMD_MOV		0x60

#define LD_CMD_ADD		0x70
#define LD_CMD_SUB		0x71
#define LD_CMD_MUL		0x72
#define LD_CMD_DIV		0x73

#define LD_CMD_NEQ		0x80
#define LD_CMD_EQU		0x81
#define LD_CMD_GRT		0x82
#define LD_CMD_GEQ		0x83
#define LD_CMD_LES		0x84
#define LD_CMD_LEQ		0x85

extern uint32_t ld_compile_pointer;

#endif /* INC_LD_EXE_H_ */
