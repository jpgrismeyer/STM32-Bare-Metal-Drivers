/*
 * 009I2C_LedToggle.c
 *
 *  Created on: Ap 05, 2025
 *      Author: @jpgrismeyer
*/

#include <stdint.h>
#include <string.h>
#include "stm32l47xx.h"
#include "stm32l475xx_gpio_driver.h"
#include "stm32l47xx_i2c_driver.h"


// Handle global para que lo vean todas las funciones si hace falta
I2C_Handle_t I2C2Handle;

void GPIO_LedInit(void) {
    GPIO_Handle_t LedPin;
    //GPIOB_PCLK_EN();

    LedPin.pGPIOx = GPIOB;
    LedPin.GPIO_PinConfig.GPIO_PinNumber = 14; // LED2 (Blue) en tu placa
    LedPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
    LedPin.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    LedPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    LedPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

    GPIO_Init(&LedPin);
}

void GPIO_ButtonInit(void) {
    GPIO_Handle_t BtnPin;
    //GPIOC_PCLK_EN();

    BtnPin.pGPIOx = GPIOC;
    BtnPin.GPIO_PinConfig.GPIO_PinNumber = 13; // User Button
    BtnPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
    BtnPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    BtnPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD; // Pull-up externo ya existe en PC13

    GPIO_Init(&BtnPin);
}

void I2C2_GPIOInit(void) {
    GPIO_Handle_t I2CPins;
    //GPIOB_PCLK_EN();

    I2CPins.pGPIOx = GPIOB;
    I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
    I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD; // REGLA DE ORO I2C
    I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU; // Pull-up interno (por las dudas)
    I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

    // SCL (PB10) y SDA (PB11)
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = 10;
    GPIO_Init(&I2CPins);
    I2CPins.GPIO_PinConfig.GPIO_PinNumber = 11;
    GPIO_Init(&I2CPins);
}

void I2C2_InitConfig(I2C_Handle_t *pI2C2Handle) {
    pI2C2Handle->pI2Cx = I2C2;
    pI2C2Handle->I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM; // 100000
    pI2C2Handle->I2CConfig.I2C_DeviceAddress = 0x61;       // Dirección del Master
    pI2C2Handle->I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;

    I2C_Init(pI2C2Handle);

}

void delay(void) {
    for(uint32_t i = 0; i < 250000; i++);
}

int main(void) {
	uint8_t reg_addr = 0x0F; // Registro WHO_AM_I
	    uint8_t sensor_id = 0;

	    // 1. Configuración de Relojes del Sistema
	    HSI16_ENABLE();
	    I2C2_CCLK_HSI16();

	    // 2. Inicialización de Periféricos (Usando tus funciones separadas)
	    GPIO_LedInit();
	    GPIO_ButtonInit();
	    I2C2_GPIOInit();
	    I2C2_InitConfig(&I2C2Handle);

	    while(1) {
	        // El botón PC13 en tu placa tira a GND (0) cuando se pulsa
	        if (GPIO_ReadFromInputPin(GPIOC, 13) == 0) {
	            delay(); // Debounce para no mandar 500 mensajes por un toque

	            // Paso A: Mandamos la dirección del registro que queremos leer (HTS221 = 0x5F)
	            I2C_MasterSendData(&I2C2Handle, &reg_addr, 1, 0x5F);

	            // Paso B: Leemos el resultado
	            I2C_MasterReceiveData(&I2C2Handle, &sensor_id, 1, 0x5F);

	            // Verificación: El HTS221 debe devolver 0xBC
	            if (sensor_id == 0xBC) {
	                // Parpadeo rápido de éxito en PB14
	                for (int i = 0; i < 10; i++) {
	                    GPIO_ToggleOutputPin(GPIOB, 14);
	                    for (int j = 0; j < 60000; j++); // Delay visual
	                }
	            }

	            sensor_id = 0; // Limpiamos para la próxima pulsación
	        }
	    }
	    return 0;
}
