/*
 * esp8266_proto.c
 *
 *  Created on: 05/mar/2016
 *      Author: chmod775
 */

#include "board.h"
#include <stdio.h>
#include <string.h>
#include "app_usbd_cfg.h"
#include "msc_disk.h"
#include "flash.h"
#include "gpio.h"
#include "fat12.h"
#include "ld_exe.h"
#include "gram.h"
#include "hw.h"
#include "cdc_vcom.h"
#include "debug.h"
#include "ssp_dma.h"

/* Transmit and receive ring buffers */
STATIC RINGBUFF_T txring, rxring;

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 256	/* Send */
#define UART_RRB_SIZE 32	/* Receive */

/* Transmit and receive buffers */
uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];


/**
 * @brief	UART interrupt handler using ring buffers
 * @return	Nothing
 */
void USART0_IRQHandler(void)
{
	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	Chip_UART0_IRQRBHandler(LPC_USART0, &rxring, &txring);
}

void esp8266_init() {
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 19, (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* Setup UART for 115.2K8N1 */
	Chip_UART0_Init(LPC_USART0);
	Chip_UART0_SetBaud(LPC_USART0, 115200);
	Chip_UART0_ConfigData(LPC_USART0, (UART0_LCR_WLEN8 | UART0_LCR_SBS_1BIT));
	Chip_UART0_SetupFIFOS(LPC_USART0, (UART0_FCR_FIFO_EN | UART0_FCR_TRG_LEV2));
	Chip_UART0_TXEnable(LPC_USART0);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
//	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
//	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Enable receive data and line status interrupt */
//	Chip_UART0_IntEnable(LPC_USART0, (UART0_IER_RBRINT | UART0_IER_RLSINT));

	/* Enable UART 0 interrupt */
//	NVIC_EnableIRQ(USART0_IRQn);
}

void esp8266_reset() {
	const char rstCMD[] = "AT+RST\n\r";
	Chip_UART0_SendBlocking(LPC_USART0, rstCMD, sizeof(rstCMD) - 1);

	uint8_t ack;

	/* Poll the receive ring buffer for the ESC (ASCII 27) key */
	ack = 0;
	while (ack != 'O') {
		Chip_UART0_ReadRB(LPC_USART0, &rxring, &ack, 1);
	}
}

int esp8266_connectWiFi(char *SSID, char *psw) {
	uint8_t ack;
	uint16_t bytes;

	const char stCMD[] = "AT+CWMODE=1\n\r";
	Chip_UART0_SendRB(LPC_USART0, &txring, stCMD, sizeof(stCMD) - 1);
	ack = 0;
	while (ack != 'O') {
		bytes = Chip_UART0_ReadRB(LPC_USART0, &rxring, &ack, 1);
	}

	const char cntCMD[] = "AT+CWJAP=\"";
	Chip_UART0_SendRB(LPC_USART0, &txring, cntCMD, sizeof(cntCMD) - 1);
	Chip_UART0_SendRB(LPC_USART0, &txring, SSID, strlen(SSID));
	const char cntTEMP1[] = "\",\"";
	Chip_UART0_SendRB(LPC_USART0, &txring, cntTEMP1, strlen(cntTEMP1));
	Chip_UART0_SendRB(LPC_USART0, &txring, psw, strlen(psw));
	const char cntTEMP2[] = "\"\n\r";
	Chip_UART0_SendRB(LPC_USART0, &txring, cntTEMP2, strlen(cntTEMP2));
	ack = 0;
	while (ack != 'O') {
		bytes = Chip_UART0_ReadRB(LPC_USART0, &rxring, &ack, 1);
	}
}


const char server[] = "www.alienlogic.it";
const char uri[] = "/esppost.php";

void esp8266_test() {
	uint8_t ack;

	const char testAP[] = "AT\r\n";
	Chip_UART0_SendBlocking(LPC_USART0, testAP, sizeof(testAP) - 1);

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}
}

void esp8266_cipstart() {
	uint8_t ack;

	const char cipstCMD[] = "AT+CIPSTART=\"TCP\",\"";
	Chip_UART0_SendBlocking(LPC_USART0, cipstCMD, sizeof(cipstCMD) - 1);
	Chip_UART0_SendBlocking(LPC_USART0, server, sizeof(server) - 1);
	const char cipstTEMP[] = "\",80\r\n";
	Chip_UART0_SendBlocking(LPC_USART0, cipstTEMP, sizeof(cipstTEMP) - 1);

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}
}

void esp8266_post(char *data) {
	uint8_t ack;
	char postRequest[0x200];
	sprintf(postRequest, "POST %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nContent-Length: %i\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n%s", uri, server, strlen(data), data);

	const char ciptxCMD[] = "AT+CIPSEND=";
	Chip_UART0_SendBlocking(LPC_USART0, ciptxCMD, sizeof(ciptxCMD) - 1);
	char postRequestLength[10];
	sprintf(postRequestLength, "%i\r\n", strlen(postRequest));
	Chip_UART0_SendBlocking(LPC_USART0, postRequestLength, strlen(postRequestLength));

	ack = 0;
	while (ack != '>') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}

	Chip_UART0_SendBlocking(LPC_USART0, postRequest, strlen(postRequest));

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}
}

void esp8266_cipclose() {
	uint8_t ack;

	const char cipclMD[] = "AT+CIPCLOSE\r\n";
	Chip_UART0_SendBlocking(LPC_USART0, cipclMD, sizeof(cipclMD) - 1);

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}
}



void esp8266_httppost(char *data) {
	uint8_t ack;
	uint16_t bytes;

	const char cipstCMD[] = "AT+CIPSTART=\"TCP\",\"";
	Chip_UART0_SendBlocking(LPC_USART0, cipstCMD, sizeof(cipstCMD) - 1);
	Chip_UART0_SendBlocking(LPC_USART0, server, sizeof(server) - 1);
	const char cipstTEMP[] = "\",80\r\n";
	Chip_UART0_SendBlocking(LPC_USART0, cipstTEMP, sizeof(cipstTEMP) - 1);

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}

	char postRequest[0x200];
	sprintf(postRequest, "POST %s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nContent-Length: %i\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n%s", uri, server, strlen(data), data);

	const char ciptxCMD[] = "AT+CIPSEND=";
	Chip_UART0_SendBlocking(LPC_USART0, ciptxCMD, sizeof(ciptxCMD) - 1);
	char postRequestLength[10];
	sprintf(postRequestLength, "%i\r\n", strlen(postRequest));
	Chip_UART0_SendBlocking(LPC_USART0, postRequestLength, strlen(postRequestLength));

	ack = 0;
	while (ack != '>') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}

	Chip_UART0_SendBlocking(LPC_USART0, postRequest, strlen(postRequest));

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}

	const char cipclMD[] = "AT+CIPCLOSE\r\n";
	Chip_UART0_SendBlocking(LPC_USART0, cipclMD, sizeof(cipclMD) - 1);

	ack = 0;
	while (ack != 'K') {
		Chip_UART0_Read(LPC_USART0, &ack, 1);
	}
}
