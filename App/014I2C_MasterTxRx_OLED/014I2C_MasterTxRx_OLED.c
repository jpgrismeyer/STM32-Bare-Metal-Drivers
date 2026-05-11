#include<stdio.h>
#include<string.h>
#include "stm32l47xx.h"




#define MY_ADDR 0x61;

I2C_Handle_t I2C1Handle;

/*
 * PB8-> SCL -> CN1 10
 * PB9-> SDA -> CN1 9
 */

//Initialize PB10 and PB11 as I2C pins.
void I2C1_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
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

//Initialize I2C1 with specific parameters
void I2C1_Inits(void)
{
	I2C1Handle.pI2Cx = I2C1;
	I2C1Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C1Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C1Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;
	I2C1Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_DISABLE;

	I2C_Init(&I2C1Handle);

}

//initialize GPIO as User Button (PC13)
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

//initialize GPIO as User LED2 (PB14)
void GPIO_LEDInit(void)
{
	GPIO_Handle_t GPIOLED;

	//this is btn gpio configuration
	GPIOLED.pGPIOx = GPIOB;
	GPIOLED.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	GPIOLED.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	GPIOLED.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOLED.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOLED);

}

int main(void)
{



	uint8_t oled_addr = 0x3C; // Typical OLED address
	uint8_t display_on[] = {0x00, 0xAF}; // 0x00 is Control Byte (Command), 0xAF is Display ON

    // 1. Init (Clock, GPIOs PB8/PB9, I2C1)
    	HSI16_ENABLE();
    	I2C1_CCLK_HSI16();

    	GPIO_ButtonInit();
    	GPIO_LEDInit();

    	//i2c pin inits
    	I2C1_GPIOInits();

    	//i2c peripheral configuration
    	I2C1_Inits();

    	//enable the i2c peripheral
    	I2C_PeripheralControl(I2C1,ENABLE);

    	while(1) {
    	    if (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13) == 0) {
    	        /* Debounce */
    	        for(uint32_t i=0; i<50000; i++);

    	        /* Send Display ON command */
    	        /* If this returns I2C_ERROR_NONE, your driver is alive! */
    	        uint8_t status = I2C_MasterSendData(&I2C1Handle, display_on, 2, oled_addr, I2C_SR_DISABLE);

    	        if (status == I2C_ERROR_NONE) {
    	            GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, 1); // LED ON if OLED acknowledged
    	            for(uint32_t i=0; i<50000; i++);
    	        } else {
    	            // Blink LED or handle error
    	        	GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, 0); // LED ON if OLED acknowledged
    	        	    	            for(uint32_t i=0; i<50000; i++);
    	        }

    	        while (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13) == 0);
    	    }
    	}
}
