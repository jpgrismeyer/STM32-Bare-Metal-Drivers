/*
 * 012I2C_MasterRx_testing.c
 *
 * Created on: Ap 19, 2025
 *      Author: @jpgrismeyer
 */

#include<stdio.h>
#include<string.h>
#include "stm32l47xx.h"




#define MY_ADDR 0x61;

#define SLAVE_ADDR  0x68
#define RX_BUFFER_LEN     10


void delay(void)
{
	for(uint32_t i = 0 ; i < 500000/2 ; i ++);
}

I2C_Handle_t I2C1Handle;
uint8_t rxBuffer[RX_BUFFER_LEN + 1];
uint32_t len=0;

/*
 * PB8-> SCL
 * PB9-> SDA
 */

//Initialize PB8 and PB9 as I2C pins.
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
	I2C1Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_ENABLE;

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
		// 5. Llamar a TU función para pedir los datos al ESP32
		        uint8_t status = I2C_MasterReceiveData(&I2C1Handle, rxBuffer, RX_BUFFER_LEN, SLAVE_ADDR);

		        if (status == I2C_ERROR_NONE) {
		            // ¡ÉXITO! Los datos llegaron correctamente
		            // Aseguramos que sea un string válido agregando el terminador nulo al final
		            rxBuffer[RX_BUFFER_LEN] = '\0';

		            // Aquí puedes poner un Breakpoint en tu IDE (STM32CubeIDE / Keil)
		            // para inspeccionar el contenido de 'rxBuffer'. Debería decir "Hola STM32"

		            // También podrías enviar este buffer por UART para verlo en la computadora.
		        }
		        else if (status == I2C_ERROR_NACK) {
		            // El ESP32 no respondió.
		            // Posibles causas: Está apagado, cables desconectados, o dirección I2C incorrecta.
		        }
		        else if (status == I2C_ERROR_TIMEOUT) {
		            // El bus se quedó colgado.
		            // Posibles causas: Faltan resistencias Pull-Up, o un esclavo bloqueó la línea de reloj (Clock Stretching).
		        }

		        // Esperar un segundo antes de volver a pedir datos
		        delay();

    }




	}


