/*
 * gpio.h
 *
 *  Created on: 26/nov/2015
 *      Author: chmod775
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

void LED_init();
void GPIO_init();

void GPIO_write(uint16_t data);

void GPIO_state(uint8_t index, bool state);
bool GPIO_state_read(uint8_t index);
void GPIO_direction(uint8_t index, bool isOutput);
void GPIO_pwm(uint8_t index);

#endif /* INC_GPIO_H_ */
