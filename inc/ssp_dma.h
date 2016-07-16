/*
 * ssp_dma.h
 *
 *  Created on: 09/feb/2016
 *      Author: chmod775
 */

#ifndef INC_SSP_DMA_H_
#define INC_SSP_DMA_H_

bool dmaTXSend(uint8_t *data, int bytes);
bool dmaRXReceive(uint8_t *data, uint32_t address, int bytes);
void ssp_dma_init();

#endif /* INC_SSP_DMA_H_ */
