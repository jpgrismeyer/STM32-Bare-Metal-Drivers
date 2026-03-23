/*
 * 001led_toggle.c
 *
 *  Created on: Feb 7, 2025
 *      Author: @jpgrismeyer
 */
#include "stm32l47xx.h"

void delay(void)
{
	for (uint32_t i=0; i<50000; i++);
}

int main(void){

	GPIO_Handle_t Gpioled;
	//led02= PB14
	Gpioled.pGPIOx=GPIOB;
	Gpioled.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	Gpioled.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	Gpioled.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	Gpioled.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	Gpioled.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_PeriClockControl(GPIOB, ENABLE);

	GPIO_Init(&Gpioled);

	while(1)
	{
		GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_14);

		delay();
	}

	return 0;
}

