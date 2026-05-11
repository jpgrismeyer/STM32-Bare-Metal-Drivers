/*
 *  011I2C_MasterTx_testing.c
 *
 *  Created on: Ap 12, 2025
 *      Author: @jpgrismeyer
*/


#include<stdio.h>
#include<string.h>
#include "stm32l47xx.h"

#define MY_ADDR 0x61;

#define SLAVE_ADDR  0x76

void delay(void)
{
	for(uint32_t i = 0 ; i < 500000/2 ; i ++);
}

I2C_Handle_t I2C1Handle;

//some data
uint8_t some_data[] = "We are testing I2C master Tx\n";
/*
 * PB8-> SCL
 * PB9-> SDA
 */

void I2C1_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	/*Note : Internal pull-up resistors are used */

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	/*
	 * Note : In the below line use GPIO_NO_PUPD option if you want to use external pullup resistors, then you have to use 3.3K pull up resistors
	 * for both SDA and SCL lines
	 */
	I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	I2CPins. GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	//scl
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_8;
	GPIO_Init(&I2CPins);


	//sda
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_9;

	GPIO_Init(&I2CPins);


}

void I2C1_Inits(void)
{
	I2C1Handle.pI2Cx = I2C1;
	I2C1Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C1Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C1Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;

	I2C_Init(&I2C1Handle);

}

void GPIO_ButtonInit(void)
{
	GPIO_Handle_t GPIOBtn;

	//this is btn gpio configuration
	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOBtn);

}


int main(void)
{
// 1. Configuración de Relojes del Sistema
	HSI16_ENABLE();
	I2C1_CCLK_HSI16();

	GPIO_ButtonInit();

	//i2c pin inits
	I2C1_GPIOInits();

	//i2c peripheral configuration
	I2C1_Inits();

	//enable the i2c peripheral
	I2C_PeripheralControl(I2C1,ENABLE);

	while(1)
	{
		//wait till button is pressed
		while(  GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

		//to avoid button de-bouncing related issues 200ms of delay
		delay();

		//send some data to the slave
		I2C_MasterSendData(&I2C1Handle, &some_data, strlen((char*)some_data), SLAVE_ADDR);
		//I2C_MasterSendData(&I2C2Handle,some_data,strlen((char*)some_data),SLAVE_ADDR,0);

	}

}
