/*
 * 007spi_txonly_esp32.c
 *
 *  Created on: Mar 08, 2025
 *      Author: @jpgrismeyer
 *
 * Test the SPI SendData API to send the string "Hello World" and use the below configurations
 * 1. SPI1 Master Mode
 * 2. SCLK = Max possible
 * 3. DFF =0 and DFF=1
 *
 * PA6 -> SPI1_MISO
 * PA7 -> SPI1_MOSI
 * PA5 -> SPI1_SCK
 * PA4 -> SPI1_NSS
 *
 * Alt funct. mode: 5
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l47xx.h"
#include "stm32l475xx_gpio_driver.h"
#include "stm32l475xx_spi_driver.h"   // este es el header que vos vas a crear p/ las funciones de arriba

void delay(void){
	for(uint32_t i =0; i< 500000/2; i++);
}
void SPI1_GPIOInits(void){
	GPIO_Handle_t SPIPins;

	SPIPins.pGPIOx = GPIOA;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;          //SPI mode.
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	//SCLK
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GPIO_Init(&SPIPins);

	//MOSI
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&SPIPins);

	//NSS
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_4;
	GPIO_Init(&SPIPins);

	//MISO
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&SPIPins);
}

void SPI1_Inits(void){
	SPI_Handle_t SPI1Handle;

	SPI1Handle.pSPIx = SPI1;
	SPI1Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI1Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI1Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI1Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI1Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	SPI1Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV16;
	SPI1Handle.SPIConfig.SPI_SSM = SPI_SSM_DI;

	SPI_Init(&SPI1Handle);
}

void GPIO_ButtonInit(void){

	GPIO_Handle_t GPIOBtn;
	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode=GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber=13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed=GPIO_SPEED_FAST;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl=GPIO_NO_PUPD;

	GPIO_Init(&GPIOBtn);
}
int main(void){

	char user_data[] = "Hello World";
	    uint8_t dummy = 0;

	//this function is used to initialize gpio pins as SPI1 pins
	GPIO_ButtonInit();
	SPI1_GPIOInits();

	//this function is used to initialize the SPI1 peripheral parameters
	SPI1_Inits();
	SPI_SSOEConfig(SPI1, ENABLE);

	while(1){
		while(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13));

		//to avoid the button de-bouncing related issues 200ms of delay
		delay();

		//Enable the SPI1 peripheral
		SPI_PeripheralControl(SPI1, ENABLE);
		//First send lenght information
		uint8_t data_len = (uint8_t)strlen(user_data);
		SPI_SendData(SPI1, &data_len, 1);
		//to send data
		SPI_SendData(SPI1, (uint8_t*)user_data, data_len);

		uint8_t padding_count = 32 - 1 - data_len;
		for(uint8_t i = 0; i < padding_count; i++){
			SPI_SendData(SPI1, &dummy, 1);
		}

		//lets confirm SPI is not busy
		while(SPI_GetFlagStatus(SPI1, SPI_BUSY_FLAG));

		//disable SPI1 peripheral
		SPI_PeripheralControl(SPI1, DISABLE);

	}

	return 0;
}
