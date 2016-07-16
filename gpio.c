/*
 * gpio.c
 *
 *  Created on: 26/nov/2015
 *      Author: chmod775
 */
#include "board.h"
#include <stdio.h>
#include "gpio.h"

static const uint8_t gpio_ports[16] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t gpio_pins[16] = {24, 7, 2, 6, 8, 9, 18, 19, 20, 23, 16, 22, 11, 12, 13, 14};

static const uint8_t gpio_top_ports[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t gpio_top_pins[8] = {20, 23, 16, 22, 11, 12, 13, 14};

static const uint8_t gpio_bottom_ports[8] = {1, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t gpio_bottom_pins[8] = {24, 7, 2, 6, 8, 9, 18, 19};

static const uint8_t led_run_port = 2;
static const uint8_t led_run_pin = 7;

static const uint8_t led_fault_port = 2;
static const uint8_t led_fault_pin = 5;

static const uint8_t bt_stop_port = 0;
static const uint8_t bt_stop_pin = 1;

static const uint8_t bt_start_port = 2;
static const uint8_t bt_start_pin = 2;

void LED_init() {
	// Run
	Chip_IOCON_PinMuxSet(LPC_IOCON, led_run_port, led_run_pin, (IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, led_run_port, led_run_pin);
	Chip_GPIO_SetPinState(LPC_GPIO, led_run_port, led_run_pin, false);

	// Fault
	Chip_IOCON_PinMuxSet(LPC_IOCON, led_fault_port, led_fault_pin, (IOCON_FUNC0 | IOCON_MODE_PULLDOWN) | IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, led_fault_port, led_fault_pin);
	Chip_GPIO_SetPinState(LPC_GPIO, led_fault_port, led_fault_pin, false);
}

void GPIO_init() {
	volatile uint32_t i;

	// Init the buttons as inputs
	Chip_IOCON_PinMuxSet(LPC_IOCON, bt_start_port, bt_start_pin, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, bt_start_port, bt_start_pin);

	Chip_IOCON_PinMuxSet(LPC_IOCON, bt_stop_port, bt_stop_pin, (IOCON_FUNC0 | IOCON_MODE_PULLUP));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, bt_stop_port, bt_stop_pin);

	// Init the led as inputs
	for (i = 0; i < 16; i++) {
		if ((i >= 12) && (i<= 15)) {
			Chip_IOCON_PinMuxSet(LPC_IOCON, gpio_ports[i], gpio_pins[i], (IOCON_FUNC1 | IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN));
		} else {
			Chip_IOCON_PinMuxSet(LPC_IOCON, gpio_ports[i], gpio_pins[i], (IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN));
		}
		Chip_GPIO_SetPinDIRInput(LPC_GPIO, gpio_ports[i], gpio_pins[i]);
		Chip_GPIO_SetPinState(LPC_GPIO, gpio_ports[i], gpio_pins[i], false);
	}
}

void GPIO_write(uint16_t data) {
	volatile uint32_t i;

	/* Output the data to the leds */
	for (i = 0; i < 16; i++) {
		Chip_GPIO_SetPinState(LPC_GPIO, gpio_ports[i], gpio_pins[i], (data & (0x01 << i)));
	}
}

void GPIO_state(uint8_t index, bool state) {
	Chip_GPIO_SetPinState(LPC_GPIO, gpio_ports[index], gpio_pins[index], state);
}
bool GPIO_state_read(uint8_t index) {
	return Chip_GPIO_GetPinState(LPC_GPIO, gpio_ports[index], gpio_pins[index]);
}
void GPIO_direction(uint8_t index, bool isOutput) {
	if ((index >= 12) && (index <= 15)) {
		Chip_IOCON_PinMuxSet(LPC_IOCON, gpio_ports[index], gpio_pins[index], (IOCON_FUNC1 | IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN));
	} else {
		Chip_IOCON_PinMuxSet(LPC_IOCON, gpio_ports[index], gpio_pins[index], (IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_DIGMODE_EN));
	}
	Chip_GPIO_SetPinDIR(LPC_GPIO, gpio_ports[index], gpio_pins[index], isOutput);
}
void GPIO_pwm(uint8_t index) {
	if (index == 14) { // PWM0
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_FUNC3 | IOCON_MODE_PULLUP | IOCON_DIGMODE_EN));
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 13);
	}
	if (index == 15) { // PWM1
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, (IOCON_FUNC3 | IOCON_MODE_PULLUP | IOCON_DIGMODE_EN));
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 14);
	}
	if (index == 10) { // PWM3
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, (IOCON_FUNC2 | IOCON_MODE_PULLUP | IOCON_DIGMODE_EN));
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 16);
	}
}
