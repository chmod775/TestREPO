/*
 * ssp_dma.c
 *
 *  Created on: 09/feb/2016
 *      Author: chmod775
 */
#include "board.h"
#include <stdio.h>
#include "inc/flash.h"


/* DMA send string arrays. DMA buffers must remain in memory during the DMA
   transfer. */
#define DMASENDSTRCNT   6
static char dmaSendStr[DMASENDSTRCNT][32];

/* Number of UART TX descriptors used for DMA */
#define UARTTXDESC 8

/* Next available UART TX DMA descriptor and use counter */
static volatile int nextTXDesc, countTXDescUsed, countRXDescUsed;

/* Number of UART RX descriptors used for DMA */
#define UARTRXDESC 4

/* Maximum size of each UART RX receive buffer */
#define UARTRXBUFFSIZE  8

/* UART RX receive buffers */
static uint8_t dmaRXBuffs[UARTRXDESC][UARTRXBUFFSIZE];

/* UART receive buffer that is available and availa flag */
static volatile int uartRXBuff;
static volatile bool uartRxAvail;

/* DMA descriptors must be aligned to 16 bytes */
#if defined(__CC_ARM)
__align(16) static DMA_CHDESC_T dmaTXDesc[UARTTXDESC];
__align(16) static DMA_CHDESC_T dmaRXDesc[UARTRXDESC];
#endif /* defined (__CC_ARM) */

/* IAR support */
#if defined(__ICCARM__)
#pragma data_alignment=16
static DMA_CHDESC_T dmaTXDesc[UARTTXDESC];
#pragma data_alignment=16
static DMA_CHDESC_T dmaRXDesc[UARTRXDESC];
#endif /* defined (__ICCARM__) */

#if defined( __GNUC__ )
static DMA_CHDESC_T dmaTXDesc[UARTTXDESC] __attribute__ ((aligned(16)));
static DMA_CHDESC_T dmaRXDesc[UARTRXDESC] __attribute__ ((aligned(16)));
#endif /* defined (__GNUC__) */


#define DMAREQ_CH_TX	DMAREQ_SSP1_TX
#define DMAREQ_CH_RX	DMAREQ_SSP1_RX

/* Setup DMA UART TX support, but do not queue descriptors yet */
static void dmaTXSetup(void)
{
	/* Setup UART 0 TX channel for the following configuration:
	   - Peripheral DMA request (UART 0 TX channel)
	   - Single transfer
	   - Low channel priority */
	Chip_DMA_EnableChannel(LPC_DMA, DMAREQ_CH_TX);
	Chip_DMA_EnableIntChannel(LPC_DMA, DMAREQ_CH_TX);
	Chip_DMA_SetupChannelConfig(LPC_DMA, DMAREQ_CH_TX,
								(DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY(3)));

	//countTXDescUsed = 0;
}

/* Setup DMA UART RX support, but do not queue descriptors yet */
static void dmaRXSetup(void)
{
	/* Setup UART 0 RX channel for the following configuration:
	   - Peripheral DMA request (UART 0 RX channel)
	   - Single transfer
	   - Low channel priority */
	Chip_DMA_EnableChannel(LPC_DMA, DMAREQ_CH_RX);
	Chip_DMA_EnableIntChannel(LPC_DMA, DMAREQ_CH_RX);
	Chip_DMA_SetupChannelConfig(LPC_DMA, DMAREQ_CH_RX,
								(DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY(3)));
}


/* Send data via the UART */
bool dmaTXSend(uint8_t *data, int bytes)
{
	/* Disable the DMA IRQ to prevent race conditions with shared data */
	NVIC_DisableIRQ(DMA_IRQn);

	/* This is a limited example, limit descriptor and byte count */
	if ((countTXDescUsed >= UARTTXDESC) || (bytes > 1024)) {
		/* Re-enable the DMA IRQ */
		NVIC_EnableIRQ(DMA_IRQn);

		/* All DMA descriptors are used, so just exit */
		return false;
	}
	else if (countTXDescUsed == 0) {
		/* No descriptors are currently used, so take the first one */
		nextTXDesc = 0;
	}

	/* Create a descriptor for the data */
	dmaTXDesc[countTXDescUsed].source = DMA_ADDR(data + bytes - 1);	/* Last address here */
	dmaTXDesc[countTXDescUsed].dest = DMA_ADDR(&LPC_SSP1->DR);	/* Byte aligned */

	/* If there are multiple buffers with non-contiguous addresses, they can be chained
	   together here (it is recommended to only use the DMA_XFERCFG_SETINTA on the
	   last chained descriptor). If another TX buffer needs to be sent, the DMA
	   IRQ handler will re-queue and send the buffer there without using chaining. */
	dmaTXDesc[countTXDescUsed].next = DMA_ADDR(0);

	/* Setup transfer configuration */
	dmaTXDesc[countTXDescUsed].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_1 |
										 DMA_XFERCFG_DSTINC_0 | DMA_XFERCFG_XFERCOUNT(bytes);

	/* If a transfer is currently in progress, then stop here and let the DMA
	   handler re-queue the next transfer. Otherwise, start the transfer here. */
	if (countTXDescUsed == 0) {
		/* Setup transfer descriptor and validate it */
		Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_CH_TX, &dmaTXDesc[countTXDescUsed]);

		/* Setup data transfer */
		Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_CH_TX,
									  dmaTXDesc[countTXDescUsed].xfercfg);

		Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_CH_TX);
	}

	/* Update used descriptor count */
	countTXDescUsed++;

	/* Re-enable the DMA IRQ */
	NVIC_EnableIRQ(DMA_IRQn);

	return true;
}

uint8_t sspDMAArr[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff};
uint8_t sspDMADummyTX = 0xFF;
uint8_t sspDMADummyRX;

bool dmaRXReceive(uint8_t *data, uint32_t address, int bytes) {
	/* Disable the DMA IRQ to prevent race conditions with shared data */
	NVIC_DisableIRQ(DMA_IRQn);

	/* This is a limited example, limit descriptor and byte count */
	if ((countTXDescUsed >= UARTTXDESC) || (bytes > 1024)) {
		/* Re-enable the DMA IRQ */
		NVIC_EnableIRQ(DMA_IRQn);

		/* All DMA descriptors are used, so just exit */
		return false;
	}
	else if (countTXDescUsed == 0) {
		/* No descriptors are currently used, so take the first one */
		nextTXDesc = 0;
	}

	FLASH_CS_0;

	sspDMAArr[0] = 0x03; // Page write command

	sspDMAArr[1] = (address >> 16) & 0xff;
	sspDMAArr[2] = (address >> 8) & 0xff;
	sspDMAArr[3] = address & 0xff;


	/* Create a descriptor for the data */
	dmaTXDesc[0].source = DMA_ADDR(sspDMAArr + 3);	/* Last address here */
	dmaTXDesc[0].dest = DMA_ADDR(&LPC_SSP1->DR);	/* Byte aligned */

	/* Create a descriptor for the data */
	dmaTXDesc[1].source = DMA_ADDR(&sspDMADummyTX);	/* Last address here */
	dmaTXDesc[1].dest = DMA_ADDR(&LPC_SSP1->DR);	/* Byte aligned */


	/* If there are multiple buffers with non-contiguous addresses, they can be chained
	   together here (it is recommended to only use the DMA_XFERCFG_SETINTA on the
	   last chained descriptor). If another TX buffer needs to be sent, the DMA
	   IRQ handler will re-queue and send the buffer there without using chaining. */
	dmaTXDesc[0].next = DMA_ADDR(&dmaTXDesc[1]);
	dmaTXDesc[1].next = DMA_ADDR(0);

	/* Setup transfer configuration */
	dmaTXDesc[0].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_1 |
										 DMA_XFERCFG_DSTINC_0 | DMA_XFERCFG_RELOAD | DMA_XFERCFG_XFERCOUNT(4);

	dmaTXDesc[1].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_0 |
										 DMA_XFERCFG_DSTINC_0 | DMA_XFERCFG_RELOAD | DMA_XFERCFG_XFERCOUNT(bytes);




	/* Create a descriptor for the data */
	dmaRXDesc[0].dest = DMA_ADDR(&sspDMADummyRX);	/* Last address here */
	dmaRXDesc[0].source = DMA_ADDR(&LPC_SSP1->DR);	/* Byte aligned */

	dmaRXDesc[1].dest = DMA_ADDR(data + bytes - 1);	/* Last address here */
	dmaRXDesc[1].source = DMA_ADDR(&LPC_SSP1->DR);	/* Byte aligned */

	/* If there are multiple buffers with non-contiguous addresses, they can be chained
	   together here (it is recommended to only use the DMA_XFERCFG_SETINTA on the
	   last chained descriptor). If another TX buffer needs to be sent, the DMA
	   IRQ handler will re-queue and send the buffer there without using chaining. */
	dmaRXDesc[0].next = DMA_ADDR(&dmaRXDesc[1]);
	dmaRXDesc[1].next = DMA_ADDR(0);

	/* Setup transfer configuration */
	dmaRXDesc[0].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_0 |
										 DMA_XFERCFG_DSTINC_0 | DMA_XFERCFG_RELOAD | DMA_XFERCFG_XFERCOUNT(4);

	dmaRXDesc[1].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_0 |
										 DMA_XFERCFG_DSTINC_1 | DMA_XFERCFG_RELOAD | DMA_XFERCFG_XFERCOUNT(bytes);


	/* If a transfer is currently in progress, then stop here and let the DMA
	   handler re-queue the next transfer. Otherwise, start the transfer here. */
	if (countTXDescUsed == 0) {
		/* Setup transfer descriptor and validate it */
		Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_CH_TX, &dmaTXDesc[0]);
		Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_CH_RX, &dmaRXDesc[0]);

		/* Setup data transfer */
		Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_CH_TX, dmaTXDesc[0].xfercfg);
		Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_CH_RX, dmaRXDesc[0].xfercfg);

		Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_CH_TX);
		Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_CH_RX);
	}

	/* Update used descriptor count */
	countTXDescUsed += 2;
	countRXDescUsed += 2;

	/* Re-enable the DMA IRQ */
	NVIC_EnableIRQ(DMA_IRQn);

	return true;
}


/* Clear an error on a DMA channel */
static void dmaClearChannel(DMA_CHID_T ch) {
	Chip_DMA_DisableChannel(LPC_DMA, ch);
	while ((Chip_DMA_GetBusyChannels(LPC_DMA) & (1 << ch)) != 0) {}

	Chip_DMA_AbortChannel(LPC_DMA, ch);
	Chip_DMA_ClearErrorIntChannel(LPC_DMA, ch);
}

/**
 * @brief	DMA Interrupt Handler
 * @return	None
 */
void DMA_IRQHandler(void)
{
	uint32_t errors, pending;

	/* Get DMA error and interrupt channels */
	errors = Chip_DMA_GetErrorIntChannels(LPC_DMA);
	pending = Chip_DMA_GetActiveIntAChannels(LPC_DMA);

	/* Check DMA interrupts of UART 0 TX channel */
	if ((errors | pending) & (1 << DMAREQ_CH_TX)) {
		/* Clear DMA interrupt for the channel */
		Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMAREQ_CH_TX);

		/* Handle errors if needed */
		if (errors & (1 << DMAREQ_CH_TX)) {
			/* DMA error, channel needs to be reset */
			dmaClearChannel(DMAREQ_CH_TX);
			dmaTXSetup();
		}
		else {
			/* Descriptor is consumed */
			countTXDescUsed--;
		}

		if (countTXDescUsed == 0) FLASH_CS_1;;

		/* Is another DMA descriptor waiting that was not chained? */
		if ((countTXDescUsed > 0) && false) {
			nextTXDesc++;

			/* Setup transfer descriptor and validate it */
			Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_CH_TX, &dmaTXDesc[nextTXDesc]);

			/* Setup data transfer */
			Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_CH_TX,
										  dmaTXDesc[nextTXDesc].xfercfg);

			Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_CH_TX);
		}
	}

	/* Check DMA interrupts of UART 0 RX channel */
	if ((errors | pending) & (1 << DMAREQ_CH_RX)) {
		/* Clear DMA interrupt for the channel */
		Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMAREQ_CH_RX);

		/* Handle errors if needed */
		if (errors & (1 << DMAREQ_CH_RX)) {
			/* DMA error, channel needs to be reset */
			dmaClearChannel(DMAREQ_CH_RX);
			dmaRXSetup();
		}
		else {
			countRXDescUsed--;
		}
	}
}

void ssp0_init() {
	/* SSP initialization */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 6, (IOCON_FUNC2 | IOCON_MODE_PULLUP)); /* PIO1_20 connected to SCK1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, (IOCON_FUNC1 | IOCON_MODE_PULLUP)); /* PIO1_21 connected to MISO1 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_FUNC1 | IOCON_MODE_PULLUP)); /* PIO1_22 connected to MOSI1 */

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 6);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 8);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 9);

	Chip_SSP_Init(LPC_SSP0);

	Chip_SSP_SetFormat(LPC_SSP0, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_MODE0);
	Chip_SSP_SetMaster(LPC_SSP0, true);
	Chip_SSP_Enable(LPC_SSP0);

	Chip_SSP_SetBitRate(LPC_SSP0, 5000000);

}

void ssp_dma_init() {
	//ssp0_init();

	/* DMA initialization - enable DMA clocking and reset DMA if needed */
	Chip_DMA_Init(LPC_DMA);

	/* Enable DMA controller and use driver provided DMA table for current descriptors */
	Chip_DMA_Enable(LPC_DMA);
	Chip_DMA_SetSRAMBase(LPC_DMA, DMA_ADDR(Chip_DMA_Table));

	/* Setup SSP 1 TX DMA support */
	dmaTXSetup();

	/* Setup SSP 1 RX DMA support */
	dmaRXSetup();
	//dmaRXQueue();

	/* Enable the DMA IRQ */
	NVIC_EnableIRQ(DMA_IRQn);

	LPC_SSP1->DMACR = 0x03;
}
