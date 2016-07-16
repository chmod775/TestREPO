/*
 * hw.h
 *
 *  Created on: 02/dic/2015
 *      Author: chmod775
 */

#ifndef INC_HW_H_
#define INC_HW_H_

extern uint16_t hw_marsh_input_table[];
extern uint16_t hw_marsh_output_table[];

bool hw_read_bool(uint16_t address);
void hw_write_bool(uint16_t address, bool state);

void hw_marshalling_input();
void hw_marshalling_output();

#endif /* INC_HW_H_ */
