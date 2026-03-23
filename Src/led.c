/*
 * led.c
 *
 *  Created on: Jun 7, 2025
 *      Author: admin
 */

#include "stm32l47xx.h"
#include "stm32l475xx_gpio_driver.h"

void LED_Init(void)
{
    GPIO_Handle_t GpioLed;

    GpioLed.pGPIOx = GPIOB;
    GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
    GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    GpioLed.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_MEDIUM;
    GpioLed.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    GpioLed.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

    GPIO_PeriClockControl(GPIOB, ENABLE);
    GPIO_Init(&GpioLed);
}
void LED_On(void)
{
    GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, ENABLE);
}

void LED_Off(void)
{
    GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, DISABLE);
}
