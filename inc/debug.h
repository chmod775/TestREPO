/*
 * debug.h
 *
 *  Created on: 10/gen/2016
 *      Author: chmod775
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

void DEBUG_MSG(char *str);
void DEBUG_PRINT(char *fmt, ...);
void DEBUG_HEX(uint8_t *array, unsigned int n);

#endif /* INC_DEBUG_H_ */
