/*
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

//Command codes
#define COMMAND_LED_CTRL		0x50
#define COMMAND_SENSOR_READ		0x51
#define COMMAND_LED_READ		0x52
#define COMMAND_PRINT			0x53
#define COMMAND_ID_READ			0x54

#define LED_ON					1
#define LED_OFF					0

//ESP32 Analog pins
#define ANALOG_PIN0				32
#define ANALOG_PIN1				33
#define ANALOG_PIN2				34
#define ANALOG_PIN3				35
#define ANALOG_PIN4				36

//ESP32 led pin
#define LED_PIN					2


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

uint8_t SPI_VerifyResponse(uint8_t ackbyte){

	if (ackbyte== 0xF5){
		//ack
		return 1;
	}
	return 0;
}


int main(void){


	uint8_t dummy_write = 0xff;
		uint8_t dummy_read;

		//initialise_monitor_handles();

		printf("Application is running\n");

		GPIO_ButtonInit();

		//this function is used to initialize the GPIO pins to behave as SPI2 pins
		SPI1_GPIOInits();

		//This function is used to initialize the SPI2 peripheral parameters
		SPI1_Inits();

		printf("SPI Init. done\n");

		/*
		* making SSOE 1 does NSS output enable.
		* The NSS pin is automatically managed by the hardware.
		* i.e when SPE=1 , NSS will be pulled to low
		* and NSS pin will be high when SPE=0
		*/
		SPI_SSOEConfig(SPI1,ENABLE);

		while(1)
		{
			//wait till button is pressed
			while(  GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

			//to avoid button de-bouncing related issues 200ms of delay
			delay();

			//enable the SPI2 peripheral
			SPI_PeripheralControl(SPI1,ENABLE);

		    //1. CMD_LED_CTRL  	<pin no(1)>     <value(1)>

			uint8_t commandcode = COMMAND_LED_CTRL;
			uint8_t ackbyte;
			uint8_t args[2];

			//send command
			SPI_SendData(SPI1,&commandcode,1);

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI1,&dummy_read,1);


			//Send some dummy bits (1 byte) fetch the response from the slave
			SPI_SendData(SPI1,&dummy_write,1);

			//read the ack byte received
			SPI_ReceiveData(SPI1,&ackbyte,1);

			if( SPI_VerifyResponse(ackbyte))
			{
				args[0] = LED_PIN;
				args[1] = LED_ON;

				//send arguments
				SPI_SendData(SPI1,args,2);
				// dummy read
				SPI_ReceiveData(SPI1,args,2);
				printf("COMMAND_LED_CTRL Executed\n");
			}
			//end of COMMAND_LED_CTRL




			//2. CMD_SENOSR_READ   <analog pin number(1) >

			//wait till button is pressed
			while( ! GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

			//to avoid button de-bouncing related issues 200ms of delay
			delay();

			commandcode = COMMAND_SENSOR_READ;

			//send command
			SPI_SendData(SPI1,&commandcode,1);

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI1,&dummy_read,1);


			//Send some dummy byte to fetch the response from the slave
			SPI_SendData(SPI1,&dummy_write,1);

			//read the ack byte received
			SPI_ReceiveData(SPI1,&ackbyte,1);

			if( SPI_VerifyResponse(ackbyte))
			{
				args[0] = ANALOG_PIN0;

				//send arguments
				SPI_SendData(SPI1,args,1); //sending one byte of

				//do dummy read to clear off the RXNE
				SPI_ReceiveData(SPI1,&dummy_read,1);

				//insert some delay so that slave can ready with the data
				delay();

				//Send some dummy bits (1 byte) fetch the response from the slave
				SPI_SendData(SPI1,&dummy_write,1);

				uint8_t analog_read;
				SPI_ReceiveData(SPI1,&analog_read,1);
				printf("COMMAND_SENSOR_READ %d\n",analog_read);
			}

			//3.  CMD_LED_READ 	 <pin no(1) >

			//wait till button is pressed
			while( ! GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

			//to avoid button de-bouncing related issues 200ms of delay
			delay();

			commandcode = COMMAND_LED_READ;

			//send command
			SPI_SendData(SPI1,&commandcode,1);

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI1,&dummy_read,1);

			//Send some dummy byte to fetch the response from the slave
			SPI_SendData(SPI1,&dummy_write,1);

			//read the ack byte received
			SPI_ReceiveData(SPI1,&ackbyte,1);

			if( SPI_VerifyResponse(ackbyte))
			{
				args[0] = LED_PIN;

				//send arguments
				SPI_SendData(SPI1,args,1); //sending one byte of

				//do dummy read to clear off the RXNE
				SPI_ReceiveData(SPI1,&dummy_read,1);

				//insert some delay so that slave can ready with the data
				delay();

				//Send some dummy bits (1 byte) fetch the response from the slave
				SPI_SendData(SPI1,&dummy_write,1);

				uint8_t led_status;
				SPI_ReceiveData(SPI1,&led_status,1);
				printf("COMMAND_READ_LED %d\n",led_status);

			}

			//4. CMD_PRINT 		<len(2)>  <message(len) >

			//wait till button is pressed
			while( ! GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

			//to avoid button de-bouncing related issues 200ms of delay
			delay();

			commandcode = COMMAND_PRINT;

			//send command
			SPI_SendData(SPI1,&commandcode,1);

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI1,&dummy_read,1);

			//Send some dummy byte to fetch the response from the slave
			SPI_SendData(SPI1,&dummy_write,1);

			//read the ack byte received
			SPI_ReceiveData(SPI1,&ackbyte,1);

			uint8_t message[] = "Hello ! How are you ??";
			if( SPI_VerifyResponse(ackbyte))
			{
				args[0] = strlen((char*)message);

				//send arguments
				SPI_SendData(SPI1,args,1); //sending length

				//do dummy read to clear off the RXNE
				SPI_ReceiveData(SPI1,&dummy_read,1);

				delay();

				//send message
				for(int i = 0 ; i < args[0] ; i++){
					SPI_SendData(SPI1,&message[i],1);
					SPI_ReceiveData(SPI1,&dummy_read,1);
				}

				printf("COMMAND_PRINT Executed \n");

			}

			//5. CMD_ID_READ
			//wait till button is pressed
			while( ! GPIO_ReadFromInputPin(GPIOC,GPIO_PIN_NO_13) );

			//to avoid button de-bouncing related issues 200ms of delay
			delay();

			commandcode = COMMAND_ID_READ;

			//send command
			SPI_SendData(SPI1,&commandcode,1);

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI1,&dummy_read,1);

			//Send some dummy byte to fetch the response from the slave
			SPI_SendData(SPI1,&dummy_write,1);

			//read the ack byte received
			SPI_ReceiveData(SPI1,&ackbyte,1);

			uint8_t id[11];
			uint32_t i=0;
			if( SPI_VerifyResponse(ackbyte))
			{
				//read 10 bytes id from the slave
				for(  i = 0 ; i < 10 ; i++)
				{
					//send dummy byte to fetch data from slave
					SPI_SendData(SPI1,&dummy_write,1);
					SPI_ReceiveData(SPI1,&id[i],1);
				}

				id[10] = '\0';

				printf("COMMAND_ID : %s \n",id);

			}

			//lets confirm SPI is not busy
			while( SPI_GetFlagStatus(SPI1,SPI_BUSY_FLAG) );

			//Disable the SPI2 peripheral
			SPI_PeripheralControl(SPI1,DISABLE);

			printf("SPI Communication Closed\n");
		}

		return 0;
}
