/*
 * flash.c
 *
 *  Created on: 26/nov/2015
 *      Author: chmod775
 */
#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include "inc/flash.h"

/* Tx buffer */
static uint8_t Tx_Buf[BUFFER_SIZE];

/* Rx buffer */
static uint8_t Rx_Buf[BUFFER_SIZE];

static Chip_SSP_DATA_SETUP_T xf_setup;

void FLASH_init() {
	/* SSP initialization */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 23, (IOCON_FUNC0 | IOCON_MODE_PULLUP)); /* PIO1_23 connected to SSEL1 */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 23);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 20, (IOCON_FUNC2 | IOCON_MODE_PULLUP)); /* PIO1_20 connected to SCK1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 21, (IOCON_FUNC2 | IOCON_MODE_PULLUP)); /* PIO1_21 connected to MISO1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 21, (IOCON_FUNC2 | IOCON_MODE_PULLUP)); /* PIO1_22 connected to MOSI1 */

	//Board_LED_Set(0, false);
	Chip_SSP_Init(LPC_SSP);

	Chip_SSP_SetFormat(LPC_SSP, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_MODE0);
	Chip_SSP_SetMaster(LPC_SSP, true);
	Chip_SSP_Enable(LPC_SSP);

	Chip_SSP_SetBitRate(LPC_SSP, 20000000);
}

void FLASH_write_enable() {
	FLASH_CS_0;
	Tx_Buf[0] = 0x06; // Write enable command

	xf_setup.length = 1;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	FLASH_CS_1;
}

void FLASH_write_page(uint32_t address, uint32_t length, uint8_t *page_data) {
	volatile uint32_t i;
	FLASH_CS_0;

	Tx_Buf[0] = 0x02; // Page write command

	Tx_Buf[1] = (address >> 16) & 0xff;
	Tx_Buf[2] = (address >> 8) & 0xff;
	Tx_Buf[3] = address & 0xff;

	for (i = 0; i < length; i++) {
		Tx_Buf[4 + i] = page_data[i];
	}

	xf_setup.length = 4 + length;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);

	FLASH_CS_1;
}

void FLASH_read_sector(uint32_t address, uint8_t *sector_data) {
	volatile uint32_t i;
	FLASH_CS_0;

	Tx_Buf[0] = 0x03; // Page write command

	Tx_Buf[1] = (address >> 16) & 0xff;
	Tx_Buf[2] = (address >> 8) & 0xff;
	Tx_Buf[3] = address & 0xff;

	xf_setup.length = 4;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);

	Chip_SSP_ReadFrames_Blocking(LPC_SSP, sector_data, 4096);

	FLASH_CS_1;
}

void FLASH_sector_erase(uint32_t address) {
	FLASH_CS_0;

	Tx_Buf[0] = 0x20; // Sector erase

	Tx_Buf[1] = (address >> 16) & 0xff;
	Tx_Buf[2] = (address >> 8) & 0xff;
	Tx_Buf[3] = address & 0xff;

	xf_setup.length = 4;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);

	FLASH_CS_1;
}

void FLASH_read(uint32_t address, uint32_t length, uint8_t *data) {
	volatile uint32_t i;
	FLASH_CS_0;

	Tx_Buf[0] = 0x03; // Page write command

	Tx_Buf[1] = (address >> 16) & 0xff;
	Tx_Buf[2] = (address >> 8) & 0xff;
	Tx_Buf[3] = address & 0xff;

	xf_setup.length = 4;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;
	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);

	Chip_SSP_ReadFrames_Blocking(LPC_SSP, data, length);

	FLASH_CS_1;
}


void FLASH_wait_busy() {
	FLASH_CS_0;
	Tx_Buf[0] = 0x05; // Register 1

	xf_setup.length = 1;
	xf_setup.tx_data = Tx_Buf;
	xf_setup.rx_data = Rx_Buf;

	xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
	Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);

	while (Rx_Buf[0] & 0x01) {
		xf_setup.rx_cnt = xf_setup.tx_cnt = 0;
		Chip_SSP_RWFrames_Blocking(LPC_SSP, &xf_setup);
	}
	FLASH_CS_1;
}

void FLASH_write_from_ram(uint8_t *ram_data, uint32_t flash_address, uint32_t n_pages) {
	volatile uint32_t page_cnt = 0;

	for (page_cnt = 0; page_cnt < n_pages; page_cnt++) {
		FLASH_write_enable();
		FLASH_write_page(flash_address + (0x100 * page_cnt), 0x100, &ram_data[0x100 * page_cnt]);
		FLASH_wait_busy();
	}
}
