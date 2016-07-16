/*
 * flash.h
 *
 *  Created on: 26/nov/2015
 *      Author: chmod775
 */

#ifndef FLASH_H_
#define FLASH_H_

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif


#define BUFFER_SIZE       (0x110)

#define LPC_SSP           LPC_SSP1
#define SSP_IRQ           SSP0_IRQn
#define SSPIRQHANDLER     SSP0_IRQHandler

#define FLASH_CS_1				Chip_GPIO_SetPinState(LPC_GPIO, 1, 23, true)
#define FLASH_CS_0				Chip_GPIO_SetPinState(LPC_GPIO, 1, 23, false)

void FLASH_init();

void FLASH_write_enable();
void FLASH_write_page(uint32_t address, uint32_t length, uint8_t *page_data);
void FLASH_write_from_ram(uint8_t *ram_data, uint32_t flash_address, uint32_t n_pages);

void FLASH_sector_erase(uint32_t address);

void FLASH_read(uint32_t address, uint32_t length, uint8_t *data);
void FLASH_read_sector(uint32_t address, uint8_t *sector_data);


void FLASH_wait_busy();

#endif /* FLASH_H_ */
