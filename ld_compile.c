/*
 * ld_compile.c
 *
 *  Created on: 05/dic/2015
 *      Author: chmod775
 */

#include "board.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ld_stack.h"
#include "gram.h"
#include "ld_exe.h"
#include "gpio.h"

struct arg_str {
	uint32_t raw_value;
	bool isFloat;
	bool isConst;
};

uint8_t ld_compile_buffer[0x200];
uint32_t ld_compile_pointer = 0;

void ld_new_bload(uint16_t address, uint8_t byte_value) {
	ld_compile_buffer[ld_compile_pointer++] = LD_CMD_BLOAD;

	ld_compile_buffer[ld_compile_pointer++] = address & 0xff;
	ld_compile_buffer[ld_compile_pointer++] = (address >> 8) & 0xff;

	ld_compile_buffer[ld_compile_pointer++] = byte_value;
}

void ld_new_vload(uint16_t address, uint32_t value) {
	ld_compile_buffer[ld_compile_pointer++] = LD_CMD_VLOAD;

	ld_compile_buffer[ld_compile_pointer++] = address & 0xff;
	ld_compile_buffer[ld_compile_pointer++] = (address >> 8) & 0xff;

	ld_compile_buffer[ld_compile_pointer++] = value & 0xff;
	ld_compile_buffer[ld_compile_pointer++] = (value >> 8) & 0xff;
	ld_compile_buffer[ld_compile_pointer++] = (value >> 16) & 0xff;
	ld_compile_buffer[ld_compile_pointer++] = (value >> 24) & 0xff;
}

void ld_new(uint8_t instruction, uint8_t argCount, ...) {
	va_list valist;
	volatile uint32_t i;

	ld_compile_buffer[ld_compile_pointer++] = instruction;

	/* initialize valist for num number of arguments */
	va_start(valist, argCount);

	/* access all the arguments assigned to valist */
	for (i = 0; i < argCount; i++) {
		uint16_t arg_addr = va_arg(valist, int);

		ld_compile_buffer[ld_compile_pointer++] = arg_addr & 0xff;
		ld_compile_buffer[ld_compile_pointer++] = (arg_addr >> 8) & 0xff;
	}

	/* clean memory reserved for valist */
	va_end(valist);
}

bool ld_compile_instr(char *instructionName, struct arg_str *args, uint8_t argCount) {
	uint16_t arg_addr[0x20];
	volatile uint32_t i;

	for (i = 0; i < argCount; i++) {
		if (args[i].isConst) {
			arg_addr[i] = gram_allocV();

			ld_new_vload(arg_addr[i], args[i].raw_value);
			if (args[i].isFloat) gram_settype_float(arg_addr[i]); else gram_settype_int(arg_addr[i]);
		} else {
			arg_addr[i] = gram_read_tagAddrItem(args[i].raw_value);
		}
	}

	if (strcmp(instructionName, "NO") == 0) {
		ld_new(LD_CMD_XIC, 1, arg_addr[0]);
	}
	if (strcmp(instructionName, "NC") == 0) {
		ld_new(LD_CMD_XIO, 1, arg_addr[0]);
	}

	if (strcmp(instructionName, "OSR") == 0) {
		ld_new(LD_CMD_OSR, 1, gram_temp_byte());
	}
	if (strcmp(instructionName, "OSF") == 0) {
		ld_new(LD_CMD_OSF, 1, gram_temp_byte());
	}

	if (strcmp(instructionName, "OUT") == 0) {
		ld_new(LD_CMD_OTE, 1, arg_addr[0]);
	}
	if (strcmp(instructionName, "OTS") == 0) {
		ld_new(LD_CMD_OTL, 1, arg_addr[0]);
	}
	if (strcmp(instructionName, "OTR") == 0) {
		ld_new(LD_CMD_OTU, 1, arg_addr[0]);
	}

	if (strcmp(instructionName, "TON") == 0) {
		ld_new(LD_CMD_TON, 2, arg_addr[0], gram_temp_value());
	}
	if (strcmp(instructionName, "TOF") == 0) {
		ld_new(LD_CMD_TOF, 2, arg_addr[0], gram_temp_value());
	}

	if (strcmp(instructionName, "MOV") == 0) {
		ld_new(LD_CMD_MOV, 2, arg_addr[0], arg_addr[1]);
	}

	if (strcmp(instructionName, "ADD") == 0) {
		ld_new(LD_CMD_ADD, 3, arg_addr[0], arg_addr[1], arg_addr[2]);
	}
	if (strcmp(instructionName, "SUB") == 0) {
		ld_new(LD_CMD_SUB, 3, arg_addr[0], arg_addr[1], arg_addr[2]);
	}

	if (strcmp(instructionName, "NEQ") == 0) {
		ld_new(LD_CMD_NEQ, 2, arg_addr[0], arg_addr[1]);
	}
	if (strcmp(instructionName, "EQU") == 0) {
		ld_new(LD_CMD_EQU, 2, arg_addr[0], arg_addr[1]);
	}
	if (strcmp(instructionName, "GRT") == 0) {
		ld_new(LD_CMD_GRT, 2, arg_addr[0], arg_addr[1]);
	}
	if (strcmp(instructionName, "GEQ") == 0) {
		ld_new(LD_CMD_GEQ, 2, arg_addr[0], arg_addr[1]);
	}
	if (strcmp(instructionName, "LES") == 0) {
		ld_new(LD_CMD_LES, 2, arg_addr[0], arg_addr[1]);
	}
	if (strcmp(instructionName, "LEQ") == 0) {
		ld_new(LD_CMD_LEQ, 2, arg_addr[0], arg_addr[1]);
	}

	if (strcmp(instructionName, "SRV") == 0) {
		ld_new(LD_CMD_SRV, 2, arg_addr[0], arg_addr[1]);
	}

	if (strcmp(instructionName, "DBG") == 0) {
		ld_new(LD_CMD_DBG, 2, arg_addr[0], arg_addr[1]);
	}

	return true;
}

bool ld_compile_line(char *src_line) {
	char token[0x20];
	uint8_t token_ptr = 0;

	char token_use = ' '; // I- Instruction, A- Argument

	char instr_name[0x20];

	struct arg_str args[0x20];
	uint8_t args_count = 0;

	char ch = *src_line;
	if (ch == 0x00) return false;

	ld_new(LD_CMD_NEWRUNG, 0);

	do {
		if (ch == '(') {
			ld_new(LD_CMD_SS, 0);
			token_ptr = 0;
			instr_name[0] = 0x00;
			token_use = 'I';
		} else if (ch == ')') {
			ld_compile_instr(instr_name, args, args_count);
			ld_new(LD_CMD_SE, 0);
			token_ptr = 0;
			instr_name[0] = 0x00;
			token_use = 'I';
		} else if (ch == '{') {
			ld_new(LD_CMD_PS, 0);
			token_ptr = 0;
			instr_name[0] = 0x00;
			token_use = 'I';
		} else if (ch == '}') {
			ld_compile_instr(instr_name, args, args_count);
			ld_new(LD_CMD_PE, 0);
			token_ptr = 0;
			instr_name[0] = 0x00;
			token_use = 'I';
		} else if (ch == '[') { // Argument part start
			token[token_ptr++] = 0x00;
			strcpy(instr_name, token);
			token_ptr = 0;
			args_count = 0;
			token_use = 'A';
		} else if (ch == ']') { // Argument part end
			if (token_ptr > 0) {
				token[token_ptr++] = 0x00;
				if (token[0] == '#') { // Const
					args[args_count].isConst = true;

					if (strchr(token, '.') != NULL) { // Float const
						float t = strtof(token + 1, NULL);

						args[args_count].raw_value = t;
						args[args_count].isFloat = true;
						args_count++;
					} else { // Int const
						args[args_count].raw_value = strtol(token + 1, NULL, 10);
						args[args_count].isFloat = false;
						args_count++;
					}
				} else {
					args[args_count].isConst = false;
					args[args_count++].raw_value = strtol(token, NULL, 10);
				}
			}
			token_ptr = 0;
			token_use = 'I';
		} else if (ch == ' ') { // Next instruction
			if (strlen(instr_name) > 0) {
				ld_compile_instr(instr_name, args, args_count);
			}
			token_ptr = 0;
			token_use = 'I';
		} else if (ch == ',') { // Next argument
			token[token_ptr++] = 0x00;

			if (token[0] == '#') { // Const
				args[args_count].isConst = true;

				if (strchr(token, '.') != NULL) { // Float const
					float t = strtof(token + 1, NULL);

					args[args_count].raw_value = t;
					args[args_count].isFloat = true;
					args_count++;
				} else { // Int const
					args[args_count].raw_value = strtol(token + 1, NULL, 10);
					args[args_count].isFloat = false;
					args_count++;
				}
			} else {
				args[args_count].isConst = false;
				args[args_count++].raw_value = strtol(token, NULL, 10);
			}

			token_ptr = 0;
			token_use = 'A';
		} else {
			token[token_ptr++] = ch;
		}

		src_line++;
		ch = *src_line;
	} while (ch != 0x00);

	return true;
}

void ld_compile_finish() {
	ld_new(LD_CMD_ENDPROG, 0);

	memcpy(ld_buffer, ld_compile_buffer, 0x200);
	memset(ld_compile_buffer, 0x00, 0x200);
}
