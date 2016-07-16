/*
===============================================================================
 Name        : PocketPLC.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
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

extern uint8_t g_memDiskArea[];

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static USBD_HANDLE_T g_hUsb;

/* Endpoint 0 patch that prevents nested NAK event processing */
static uint32_t g_ep0RxBusy = 0;/* flag indicating whether EP0 OUT/RX buffer is busy. */
static USB_EP_HANDLER_T g_Ep0BaseHdlr;	/* variable to store the pointer to base EP0 handler */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
const USBD_API_T *g_pUsbApi;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* EP0_patch part of WORKAROUND for artf45032. */
ErrorCode_t EP0_patch(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	switch (event) {
	case USB_EVT_OUT_NAK:
		if (g_ep0RxBusy) {
			/* we already queued the buffer so ignore this NAK event. */
			return LPC_OK;
		}
		else {
			/* Mark EP0_RX buffer as busy and allow base handler to queue the buffer. */
			g_ep0RxBusy = 1;
		}
		break;

	case USB_EVT_SETUP:	/* reset the flag when new setup sequence starts */
	case USB_EVT_OUT:
		/* we received the packet so clear the flag. */
		g_ep0RxBusy = 0;
		break;
	}
	return g_Ep0BaseHdlr(hUsb, data, event);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from USB0
 * @return	Nothing
 */
void USB_IRQHandler(void)
{
	uint32_t *addr = (uint32_t *) LPC_USB->EPLISTSTART;

	/*	WORKAROUND for artf32289 ROM driver BUG:
	    As part of USB specification the device should respond
	    with STALL condition for any unsupported setup packet. The host will send
	    new setup packet/request on seeing STALL condition for EP0 instead of sending
	    a clear STALL request. Current driver in ROM doesn't clear the STALL
	    condition on new setup packet which should be fixed.
	 */
	if ( LPC_USB->DEVCMDSTAT & _BIT(8) ) {	/* if setup packet is received */
		addr[0] &= ~(_BIT(29));	/* clear EP0_OUT stall */
		addr[2] &= ~(_BIT(29));	/* clear EP0_IN stall */
	}
	USBD_API->hw->ISR(g_hUsb);
}

int pwm_test = 9000;

/**
 * @brief	Find the address of interface descriptor for given class type.
 * @return	If found returns the address of requested interface else returns NULL.
 */
USB_INTERFACE_DESCRIPTOR *find_IntfDesc(const uint8_t *pDesc, uint32_t intfClass)
{
	USB_COMMON_DESCRIPTOR *pD;
	USB_INTERFACE_DESCRIPTOR *pIntfDesc = 0;
	uint32_t next_desc_adr;

	pD = (USB_COMMON_DESCRIPTOR *) pDesc;
	next_desc_adr = (uint32_t) pDesc;

	while (pD->bLength) {
		/* is it interface descriptor */
		if (pD->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) {

			pIntfDesc = (USB_INTERFACE_DESCRIPTOR *) pD;
			/* did we find the right interface descriptor */
			if (pIntfDesc->bInterfaceClass == intfClass) {
				break;
			}
		}
		pIntfDesc = 0;
		next_desc_adr = (uint32_t) pD + pD->bLength;
		pD = (USB_COMMON_DESCRIPTOR *) next_desc_adr;
	}

	return pIntfDesc;
}

bool run_state = false;

bool file_error = false;
uint8_t led_blink_cnt = 0;

uint8_t temp[0x200];

bool newCompile = false;
uint16_t postCnt = 0;

void SysTick_Handler(void)
{
	if ((led_blink_cnt & 0x10) && ((led_blink_cnt & 0x0f) == 0)) {
		if (run_state) {
			if (file_error) {
				Chip_GPIO_WritePortBit(LPC_GPIO, 2, 7, !Chip_GPIO_ReadPortBit(LPC_GPIO, 2, 7));
			} else {
				Chip_GPIO_WritePortBit(LPC_GPIO, 2, 5, !Chip_GPIO_ReadPortBit(LPC_GPIO, 2, 5));
			}
		} else {
			Chip_GPIO_WritePortBit(LPC_GPIO, 2, 7, true);
		}
	}

	if (run_state) ld_cycle();

	if ((!Chip_GPIO_GetPinState(LPC_GPIO, 2, 2)) && (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 1)) && !newCompile) {
		ld_compile_file("PRJ1");
		Chip_GPIO_WritePortBit(LPC_GPIO, 2, 5, true);
		Chip_GPIO_WritePortBit(LPC_GPIO, 2, 7, true);
		newCompile = true;
	} else {
		newCompile = false;
		if (!Chip_GPIO_GetPinState(LPC_GPIO, 2, 2)) { // Stop
			if (run_state == true) {
				DEBUG_PRINT("Compiled %i instructions!\n\r", ld_compile_pointer);
				DEBUG_MSG("Stop mode");

				DEBUG_HEX(temp, 0x200);

				//FLASH_read(0, 0x200, temp);
				//DEBUG_HEX(temp, 0x200);

				//dmaTXSend(temp, 0x100);
				DEBUG_MSG("CODE:");
				dmaRXReceive(temp, 0, 0x200);
				DEBUG_HEX(ld_buffer, 0x200);
			}
			run_state = false;
			Chip_GPIO_WritePortBit(LPC_GPIO, 2, 5, false);
			Chip_GPIO_WritePortBit(LPC_GPIO, 2, 7, false);
		}
		if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 1)) { // Run
			//if (run_state == false) ld_compile_file("PRJ1");

			if (run_state == false) {
				DEBUG_MSG("Run mode");
				DEBUG_HEX(gramTYPE, 0x100);
				//DEBUG_PRINT("[T] Float: %f \n\r", 2.7182);
			}
			run_state = true;
			Chip_GPIO_WritePortBit(LPC_GPIO, 2, 5, false);
			Chip_GPIO_WritePortBit(LPC_GPIO, 2, 7, false);
		}
	}

	led_blink_cnt++;

	if (led_blink_cnt == 255) {
		char t[50];
		sprintf(t, "value=%i", postCnt);
		//esp8266_httppost(t);
		//Chip_GPIO_WritePortBit(LPC_GPIO, 2, 5, !Chip_GPIO_ReadPortBit(LPC_GPIO, 2, 5));
		postCnt++;
	}
}

void Init_PWM(uint16_t prescaler, uint16_t OnResetValue)
{
    /* Setup Timer for PWM */
    Chip_TIMER_Init(LPC_TIMER32_1);
    // MR2 reset
    Chip_TIMER_ResetOnMatchEnable(LPC_TIMER32_1, 2);
    // Set the frequency prescaler for the timer
    Chip_TIMER_PrescaleSet(LPC_TIMER32_1, prescaler-1);
    // Set MR2 value for resetting the timer
    Chip_TIMER_SetMatch(LPC_TIMER32_1, 2, OnResetValue);
    // Set PWM Control Register
    Chip_TIMER_PWMWrite(LPC_TIMER32_1,(1<<0 | 1<<1 | 1<<3));
    // Enable Timer16_0
    Chip_TIMER_Enable(LPC_TIMER32_1);
}


#define SRC_IO		1
#define SRC_TAGS	2
#define SRC_INIT	3
#define SRC_CYCLE	4

int ld_compile_file(char *filename) {
	FAT12_stream f_src = FAT12_root_fopen(filename);
	file_error = !FAT12_root_fexist(filename);

	if (file_error) return -1;

	char src_line[0x200];

	// 0- UNKNOWN
	// 1- IO configuration
	// 2- INIT variables (unused)
	// 3- CYCLE code
	uint8_t src_part = 0;

	gram_reset();

	do {
		FAT12_fread_line(&f_src, src_line);

		if (strcmp(src_line, "#IO") == 0) {
			src_part = SRC_IO;
		} else if (strcmp(src_line, "#TAGS") == 0) {
			src_part = SRC_TAGS;
		} else if (strcmp(src_line, "#INIT") == 0) {
			src_part = SRC_INIT;
		} else if (strcmp(src_line, "#CYCLE") == 0) {
			src_part = SRC_CYCLE;
		} else {
			if (src_part == SRC_TAGS) {
				char *tag_ptr = strtok(src_line, ":");
				char *type_ptr = strtok(NULL, ":");
				uint16_t *tagAddrItem = gram_tagAddrItem();

				if (type_ptr[0] == 'B')	*tagAddrItem = gram_allocB();
				if (type_ptr[0] == 'V')	*tagAddrItem = gram_allocV();

				DEBUG_PRINT("    %c @ %i\n\r", type_ptr[0], *tagAddrItem);
			} else if (src_part == SRC_IO) {
				char *index_ptr = strtok(src_line, ":");
				char *dir_ptr = strtok(NULL, ":");
				char *tag_ptr = strtok(NULL, ":");
				char *addr_ptr = strtok(NULL, ":");

				uint8_t index =  strtol(index_ptr, NULL, 10);

				if (dir_ptr[0] == 'P') {
					GPIO_pwm(index);
				} else {
					if (dir_ptr[0] == 'O')
						hw_marsh_output_table[index] = gram_read_tagAddrItem(strtol(addr_ptr, NULL, 10));
					else
						hw_marsh_input_table[index] = gram_read_tagAddrItem(strtol(addr_ptr, NULL, 10));

					GPIO_state(index, false);

					if (dir_ptr[0] == 'O')	GPIO_direction(index, true);
					if (dir_ptr[0] == 'I')	GPIO_direction(index, false);
				}
			} else if (src_part == SRC_INIT) {

			} else if (src_part == SRC_CYCLE) {
				ld_compile_line(src_line);
			} else {}
		}

	} while (!f_src.eof);

	ld_compile_finish();

	return 0;
}



/**
 * @brief	main routine for blinky example
 * @return	Function should not exit.
 */
int main(void)
{
	extern void DataRam_Initialize(void);
	USBD_API_INIT_PARAM_T usb_param;
	USB_CORE_DESCS_T desc;
	ErrorCode_t ret = LPC_OK;
	USB_CORE_CTRL_T *pCtrl;

	/* Initialize board and chip */
	SystemCoreClockUpdate();
	Board_Init();

	FLASH_init();

	/* Init the GPIO and the LEDs */
	LED_init();
	GPIO_init();

	/* enable clocks and pinmux */
	Chip_USB_Init();


	FAT12_init();

	DEBUG_MSG("Compiling PRJ1.txt ...");
	int ld_ret = ld_compile_file("PRJ1");
	if (ld_ret == 0) DEBUG_PRINT("Compiled %d instructions!", ld_compile_pointer); else DEBUG_MSG("ERROR!");

	gram_clear();

	/* Init USB API structure */
	g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->usbdApiBase;

	/* initialize call back structures */
	memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
	usb_param.usb_reg_base = LPC_USB0_BASE;
	usb_param.mem_base = USB_STACK_MEM_BASE;
	usb_param.mem_size = USB_STACK_MEM_SIZE;
	usb_param.max_num_ep = 6 + 1;

	/* Set the USB descriptors */
	desc.device_desc = (uint8_t *) USB_DeviceDescriptor;
	desc.string_desc = (uint8_t *) USB_StringDescriptor;

	/* Note, to pass USBCV test full-speed only devices should have both
	   descriptor arrays point to same location and device_qualifier set to 0.
	 */
	desc.high_speed_desc = USB_FsConfigDescriptor;
	desc.full_speed_desc = USB_FsConfigDescriptor;
	desc.device_qualifier = 0;

	/* USB Initialization */
	ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
	if (ret == LPC_OK) {
		/*	WORKAROUND for artf32219 ROM driver BUG:
		    The mem_base parameter part of USB_param structure returned
		    by Init() routine is not accurate causing memory allocation issues for
		    further components.
		 */
		usb_param.mem_base = USB_STACK_MEM_BASE + (USB_STACK_MEM_SIZE - usb_param.mem_size);

		/*	WORKAROUND for artf45032 ROM driver BUG:
		    Due to a race condition there is the chance that a second NAK event will
		    occur before the default endpoint0 handler has completed its preparation
		    of the DMA engine for the first NAK event. This can cause certain fields
		    in the DMA descriptors to be in an invalid state when the USB controller
		    reads them, thereby causing a hang.
		 */
		pCtrl = (USB_CORE_CTRL_T *) g_hUsb;	/* convert the handle to control structure */
		g_Ep0BaseHdlr = pCtrl->ep_event_hdlr[0];/* retrieve the default EP0_OUT handler */
		pCtrl->ep_event_hdlr[0] = EP0_patch;/* set our patch routine as EP0_OUT handler */

		ret = mscDisk_init(g_hUsb, &desc, &usb_param);
		if (ret == LPC_OK) {
			/* Init VCOM interface */
			ret = vcom_init(g_hUsb, &desc, &usb_param);
			if (ret == LPC_OK) {
				/*  enable USB interrupts */
				NVIC_EnableIRQ(USB0_IRQn);
				/* now connect */
				USBD_API->hw->Connect(g_hUsb, 1);
			}
		}
	}

	/* SSP DMA */
	ssp_dma_init();

	esp8266_init();
	//esp8266_reset();
	esp8266_test();
	esp8266_test();
	//esp8266_cipstart();

	//esp8266_httppost("value=HelloWorld");

	/* Enable and setup SysTick Timer at a periodic rate */
	SysTick_Config(SystemCoreClock / 200);

	//ld_cycle_loop();

	//SCT1_Init();

    uint16_t pwm_value = 1800;
    Init_PWM(96, 10000);

    // set MATCH2(PIO0_10) to a duty cycle of 50%
//    Chip_TIMER_SetMatch(LPC_TIMER32_1, 0, pwm_test);
//    Chip_TIMER_SetMatch(LPC_TIMER32_1, 1, 9250);
//    Chip_TIMER_SetMatch(LPC_TIMER32_1, 3, 0);

    int prompt = 0, rdCnt = 0;
    static uint8_t g_rxBuff[256];

	while (1) {
		/* Check if host has connected and opened the VCOM port */
		if ((vcom_connected() != 0) && (prompt == 0)) {
			vcom_write("Hello World!!\r\n", 15);
			prompt = 1;
		}
		/* If VCOM port is opened echo whatever we receive back to host. */
		if (prompt) {
			rdCnt = vcom_bread(&g_rxBuff[0], 256);
			if (rdCnt) {
				vcom_write(&g_rxBuff[0], rdCnt);
			}
		}

		/* Sleep until next IRQ happens */
		__WFI();
	}
}
