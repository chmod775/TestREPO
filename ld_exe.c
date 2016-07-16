/*
 * ld_exe.c
 *
 *  Created on: 30/nov/2015
 *      Author: Codematica
 */
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "ld_stack.h"
#include "gram.h"
#include "ld_exe.h"
#include "gpio.h"
#include "hw.h"

#define LD_BUFFER_SIZE	0x200
uint8_t ld_buffer[LD_BUFFER_SIZE];
uint32_t ld_buffer_pointer = 0;

uint8_t ld_read_byte() {
	uint8_t arg_value = ld_buffer[ld_buffer_pointer];
	ld_buffer_pointer += 1;
	return arg_value;
}
uint16_t ld_read_argument() {
	uint16_t arg_value = ld_buffer[ld_buffer_pointer] | (ld_buffer[ld_buffer_pointer + 1] << 8);
	ld_buffer_pointer += 2;
	return arg_value;
}
uint32_t ld_read_value() {
	uint32_t arg_value = ld_buffer[ld_buffer_pointer] | (ld_buffer[ld_buffer_pointer + 1] << 8) | (ld_buffer[ld_buffer_pointer + 2] << 16) | (ld_buffer[ld_buffer_pointer + 3] << 24);
	ld_buffer_pointer += 4;
	return arg_value;
}

void ld_cmd_xic() {
	uint16_t tag_addr = ld_read_argument();
	ld_stack_load(gram_read_bool(tag_addr));
}
void ld_cmd_xio() {
	uint16_t tag_addr = ld_read_argument();
	ld_stack_load(!gram_read_bool(tag_addr));
}

void ld_cmd_ote() {
	uint16_t tag_addr = ld_read_argument();
	gram_write_bool(tag_addr, ld_stack_result());
}
void ld_cmd_otl() {
	uint16_t tag_addr = ld_read_argument();
	bool status = ld_stack_result();
	if (status) gram_write_bool(tag_addr, true);
}
void ld_cmd_otu() {
	uint16_t tag_addr = ld_read_argument();
	bool status = ld_stack_result();
	if (status) gram_write_bool(tag_addr, false);
}

void ld_cmd_vload() { // address, value
	uint16_t tag_addr = ld_read_argument();
	uint32_t value = ld_read_value();
	gram_write_value(tag_addr, value);
}
void ld_cmd_bload() { // address, byteData
	uint16_t tag_addr = ld_read_argument();
	uint8_t byte = ld_read_byte();
	gram_write_bool(tag_addr, byte);
}

void ld_cmd_ton() { // init, tempCounter
	uint16_t init_addr = ld_read_argument();
	uint16_t cnt_addr = ld_read_argument();

	uint32_t init_value = gram_read_value(init_addr);

	bool status = ld_stack_result();
	ld_stack_close();

	if (status) {
		uint32_t cnt_value = gram_read_value(cnt_addr);

		if (cnt_value > 0) {
			cnt_value--;
			gram_write_value(cnt_addr, cnt_value);
			ld_stack_load(false);
		} else {
			ld_stack_load(true);
		}
	} else {
		gram_write_value(cnt_addr, init_value);
		ld_stack_load(false);
	}
}
void ld_cmd_tof() { // init, tempCounter
	uint16_t init_addr = ld_read_argument();
	uint16_t cnt_addr = ld_read_argument();

	uint32_t init_value = gram_read_value(init_addr);

	bool status = ld_stack_result();
	ld_stack_close();

	if (!status) {
		uint32_t cnt_value = gram_read_value(cnt_addr);

		if (cnt_value > 0) {
			cnt_value--;
			gram_write_value(cnt_addr, cnt_value);
			ld_stack_load(true);
		} else {
			ld_stack_load(false);
		}
	} else {
		gram_write_value(cnt_addr, init_value);
		ld_stack_load(true);
	}
}

void ld_cmd_osr() {
	uint16_t temp_addr = ld_read_argument();
	bool status = ld_stack_result();
	ld_stack_close();

	bool temp_value = gram_read_bool(temp_addr);

	ld_stack_load((status != temp_value) && (status == true));
	gram_write_bool(temp_addr, status);
}
void ld_cmd_osf() {
	uint16_t temp_addr = ld_read_argument();
	bool status = ld_stack_result();

	bool temp_value = gram_read_bool(temp_addr);

	ld_stack_load(((status != temp_value) && (status == false)));
	gram_write_bool(temp_addr, status);
}

void ld_cmd_srv() {
	uint16_t servo_addr = ld_read_argument();
	uint16_t pos_addr = ld_read_argument();

	uint32_t servo_value = gram_read_value(servo_addr);
	uint32_t pos_value = gram_read_value(pos_addr);

	uint32_t pos_pwm = (pos_value * 10) + 8750;

    if (ld_stack_result()) Chip_TIMER_SetMatch(LPC_TIMER32_1, servo_value, pos_pwm);
}

void ld_cmd_mov() {
	uint16_t dst_addr = ld_read_argument();
	uint16_t src_addr = ld_read_argument();

	uint32_t src_value = gram_read_value(src_addr);

    if (ld_stack_result()) {
    	gram_write_value(dst_addr, src_value);
    	if (gram_gettype(src_addr)) gram_settype_float(dst_addr); else gram_settype_int(dst_addr);
   }
}

void ld_cmd_add() {
	uint16_t dst_addr = ld_read_argument();
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	if (gram_gettype(n1_addr) | gram_gettype(n2_addr)) {
		float n1_value = gram_read_float_forced(n1_addr);
		float n2_value = gram_read_float_forced(n2_addr);

	    if (ld_stack_result()) gram_write_float(dst_addr, n1_value + n2_value);
	} else { // Int + Int
		uint32_t n1_value = gram_read_int(n1_addr);
		uint32_t n2_value = gram_read_int(n2_addr);

	    if (ld_stack_result()) gram_write_int(dst_addr, n1_value + n2_value);
	}
}
void ld_cmd_sub() {
	uint16_t dst_addr = ld_read_argument();
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

    if (ld_stack_result()) gram_write_value(dst_addr, n1_value - n2_value);
}

void ld_cmd_neq() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value != n2_value);
}
void ld_cmd_equ() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value == n2_value);
}
void ld_cmd_grt() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value > n2_value);
}
void ld_cmd_geq() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value >= n2_value);
}
void ld_cmd_les() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value < n2_value);
}
void ld_cmd_leq() {
	uint16_t n1_addr = ld_read_argument();
	uint16_t n2_addr = ld_read_argument();

	uint32_t n1_value = gram_read_value(n1_addr);
	uint32_t n2_value = gram_read_value(n2_addr);

	ld_stack_load(n1_value <= n2_value);
}

void ld_cmd_dbg() {
	uint16_t ref_addr = ld_read_argument();
	uint16_t arg_addr = ld_read_argument();

	uint32_t ref_value = gram_read_int(ref_addr);

	if (ld_stack_result()) {
		if (gram_gettype(arg_addr)) { // Float
			DEBUG_PRINT("[%d] Float: %f\n\r", ref_value, gram_read_float(arg_addr));
		} else { // Int
			DEBUG_PRINT("[%d] Int: %d\n\r", ref_value, gram_read_int(arg_addr));
		}
	}
}

const void (*instr[256])(void) = {
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


bool ld_exe() {
	uint8_t ld_instruction = ld_buffer[ld_buffer_pointer];
	ld_buffer_pointer++;

	if (ld_instruction == LD_CMD_ENDPROG) return false;
	if (ld_instruction == LD_CMD_NEWRUNG) ld_stack_clear();

	if (ld_instruction == LD_CMD_SS) ld_stack_ss();
	if (ld_instruction == LD_CMD_SE) ld_stack_se();
	if (ld_instruction == LD_CMD_PS) ld_stack_ps();
	if (ld_instruction == LD_CMD_PE) ld_stack_pe();

	if (ld_instruction == LD_CMD_XIC) ld_cmd_xic();
	if (ld_instruction == LD_CMD_XIO) ld_cmd_xio();

	if (ld_instruction == LD_CMD_VLOAD) ld_cmd_vload();
	if (ld_instruction == LD_CMD_BLOAD) ld_cmd_bload();

	if (ld_instruction == LD_CMD_OTE) ld_cmd_ote();
	if (ld_instruction == LD_CMD_OTL) ld_cmd_otl();
	if (ld_instruction == LD_CMD_OTU) ld_cmd_otu();

	if (ld_instruction == LD_CMD_TON) ld_cmd_ton();
	if (ld_instruction == LD_CMD_TOF) ld_cmd_tof();

	if (ld_instruction == LD_CMD_OSR) ld_cmd_osr();
	if (ld_instruction == LD_CMD_OSF) ld_cmd_osf();

	if (ld_instruction == LD_CMD_SRV) ld_cmd_srv();

	if (ld_instruction == LD_CMD_MOV) ld_cmd_mov();

	if (ld_instruction == LD_CMD_ADD) ld_cmd_add();
	if (ld_instruction == LD_CMD_SUB) ld_cmd_sub();

	if (ld_instruction == LD_CMD_NEQ) ld_cmd_neq();
	if (ld_instruction == LD_CMD_EQU) ld_cmd_equ();
	if (ld_instruction == LD_CMD_GRT) ld_cmd_grt();
	if (ld_instruction == LD_CMD_GEQ) ld_cmd_geq();
	if (ld_instruction == LD_CMD_LES) ld_cmd_les();
	if (ld_instruction == LD_CMD_LEQ) ld_cmd_leq();

	if (ld_instruction == LD_CMD_DBG) ld_cmd_dbg();

	return true;
}

void ld_cycle() {
	bool ret = true;
	ld_buffer_pointer = 0;

	hw_marshalling_input();

	do {
		ret = ld_exe();
	} while (ret);

	hw_marshalling_output();
}

void ld_cycle_loop() {
	bool ret = true;
	while (1) {
		ret = true;
		ld_buffer_pointer = 0;
		do {
			ret = ld_exe();
		} while (ret);
	}
}
