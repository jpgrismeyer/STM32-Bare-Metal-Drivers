/*
 * 005button_interrupt.c
 *
 *  Created on: Feb 15, 2025
 *      Author: @jpgrismeyer
 */

#include "stm32l47xx.h"

volatile uint8_t g_button_pressed = 0; // 'volatile' es clave aquí

void delay(void)
{
	for (uint32_t i=0; i<50000; i++);
}

int main(void){
GPIO_Handle_t Gpioled,GPIOBtn;
	//led02= PB14
	Gpioled.pGPIOx=GPIOB;
	Gpioled.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	Gpioled.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	Gpioled.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	Gpioled.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	Gpioled.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_PeriClockControl(GPIOB, ENABLE);

	GPIO_Init(&Gpioled);

	GPIOBtn.pGPIOx=GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IT_FT;
	GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;

	GPIO_PeriClockControl(GPIOC, ENABLE);

	GPIO_Init(&GPIOBtn);


	//IRQ configurations

	GPIO_IRQPriorityConfig(IRQ_NO_EXTI15_10, NVIC_IRQ_PRI15);
	GPIO_IRQInterruptConfig(IRQ_NO_EXTI15_10, ENABLE);

	while(1)
		if(g_button_pressed)
		{
			delay();
			// Acción: Toggle del LED en PB14
			GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_14);

			// Reset del flag de software
			g_button_pressed = 0;
		}
return 0;

}
void EXTI15_10_IRQHandler (void) //from startup file
{
	// 1. Limpiamos el flag de hardware para la línea 13
	    GPIO_IRQHandling(GPIO_PIN_NO_13);

	    // 2. Avisamos al main que hubo un evento (Operación ultra rápida)
	    g_button_pressed = 1;

}

