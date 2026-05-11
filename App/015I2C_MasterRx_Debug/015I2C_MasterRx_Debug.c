#include<stdio.h>
#include<string.h>
#include "stm32l47xx.h"





// Definitions for LPS22HB Integrated Sensor in Discovery Board
#define LPS22HB_ADDR      0x5D
#define WHO_AM_I_REG      0x0F
#define EXPECTED_VALUE    0xB1
#define MY_ADDR 0x61;

I2C_Handle_t I2C2Handle;
USART_Handle_t USART1Handle;



//Initialize PB10 and PB11 as I2C pins.
void I2C2_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	I2CPins. GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	//scl
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
	GPIO_Init(&I2CPins);


	//sda
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
	GPIO_Init(&I2CPins);


}

//Initialize I2C1 with specific parameters
void I2C2_Inits(void)
{
	I2C2Handle.pI2Cx = I2C2;
	I2C2Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C2Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C2Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;
	I2C2Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_DISABLE;

	I2C_Init(&I2C2Handle);

}

//Initialize PB6 and PB7 as USART pins.
void USART1_GPIOInits(void)
{
	GPIO_Handle_t USARTPins;

	USARTPins.pGPIOx = GPIOB;
	USARTPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	USARTPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;     //siempre para USART
	USARTPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	USARTPins.GPIO_PinConfig.GPIO_PinAltFunMode = 7;
	USARTPins. GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	//Usart1 Tx
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&USARTPins);


	//Usart1 Rx
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&USARTPins);


}

//Initialize USART1 with specific parameters
void USART1_Inits(void)
{
	USART1Handle.pUSARTx = USART1;
	USART1Handle.USART_Config.USART_Baud = USART_STD_BAUD_9600;
	USART1Handle.USART_Config.USART_Mode = USART_MODE_ONLY_TX;
	USART1Handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
	USART1Handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;

	USART_Init(&USART1Handle);

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

//	 uint8_t msg[] = "HELLO\r\n";

    uint8_t reg_addr = WHO_AM_I_REG;
    uint8_t who_am_i_value = 0;
    uint8_t msg_ok[] = "WHOAMI:OK\n";
    uint8_t msg_fail[] = "WHOAMI:FAIL\n";

    // 1. Init (Clock, GPIOs PB10/PB11, I2C2)

	SystemClock_HSI_Init();

	/*
	 * Enable HSI16 because it will be used as the USART1 kernel clock.
	 */
	HSI16_ENABLE();
	/*
	 * Select HSI16 as USART1 clock source.
	 * USART1SEL bits are RCC_CCIPR[1:0]:
	 * 00: PCLK
	 * 01: SYSCLK
	 * 10: HSI16
	 * 11: LSE
	 */
	USART1_CCLK_HSI16();  // HSI16
	I2C2_CCLK_HSI16();

	GPIO_ButtonInit();
	GPIO_LEDInit();

	//i2c pin inits
	I2C2_GPIOInits();

	//i2c peripheral configuration
	I2C2_Inits();

	//enable the i2c peripheral
	I2C_PeripheralControl(I2C2,ENABLE);

	USART1_GPIOInits();

	USART1_Inits();

    while(1){





    if(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)==0){

        // STEP 1: Send address of the register to be read
        //  Sr = I2C_SR_ENABLE (Repeated Start) usually used with sensors
        I2C_MasterSendData(&I2C2Handle, &reg_addr, 1, LPS22HB_ADDR, I2C_SR_ENABLE);

        // STEP 2: Read register value
        // Here we end communication with Sr = DISABLE (Generates STOP)
        I2C_MasterReceiveData(&I2C2Handle, &who_am_i_value, 1, LPS22HB_ADDR, I2C_SR_DISABLE);

        // STEP 3: Validation
        if (who_am_i_value == EXPECTED_VALUE)
        {
            // Sensor responded perfectly (Value 0xB1)

            GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, ENABLE);
            USART_SendData(&USART1Handle, msg_ok, sizeof(msg_ok) - 1);
            for(volatile int i=0; i<1000000; i++);
        }
        else
        {
            // if 0x00 or 0xFF the bus is not reaching the sensorSi recibís 0x00 o 0xFF, el bus no está llegando al sensor.
            // if another value, there's an issue in driver.
        	GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, DISABLE);
        	USART_SendData(&USART1Handle, msg_fail, sizeof(msg_fail) - 1);
        	for(volatile int i=0; i<1000000; i++);
        }
        /* Wait until the button is released to prevent multiple executions
		 * This allows you to capture a single clean transaction on your logic analyzer*/
        while (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13) == 0);

		// Debounce
		for(uint32_t i=0; i<50000; i++);


    }


    }
}

