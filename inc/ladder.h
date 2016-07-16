/*
 * ladder.h
 *
 *  Created on: 29/nov/2015
 *      Author: chmod775
 */

#ifndef INC_LADDER_H_
#define INC_LADDER_H_

#define LD_TAG_BOOL		0
#define LD_TAG_VALUE	1

/*****************************/
// Tag type:
//  0- Bool  (1 Byte)
//  1- Value (4 Bytes)

struct ld_tag_str {
	char *name;
	uint8_t type;
	uint16_t address;
	bool destructive;
};



#endif /* INC_LADDER_H_ */
