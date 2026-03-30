/*
 * stm32l47xx_i2c_driver.h
 *
 *  Created on: May 24, 2025
 *      Author: admin
 */

#ifndef INC_STM32L47XX_I2C_DRIVER_H_
#define INC_STM32L47XX_I2C_DRIVER_H_


#include "stm32l47xx.h"

typedef struct
{
    uint32_t I2C_SCLSpeed;
    uint8_t  I2C_DeviceAddress;
    uint8_t  I2C_NoStretch;
	uint32_t I2C_FMDutyCycle;
} I2C_Config_t;

typedef struct
{
    I2C_RegDef_t *pI2Cx;
    I2C_Config_t I2CConfig;
} I2C_Handle_t;


/*
 * I2C Timing values para I2CCLK = 16MHz
 * Estos valores están calculados para cumplir con los tiempos de subida/bajada de la norma I2C
 */
#define I2C_TIMING_100KHZ_16MHZ    0x00303D5B
#define I2C_TIMING_400KHZ_16MHZ    0x0010061A

/* Macros para que el usuario elija en el main */
#define I2C_SCL_SPEED_SM           100000  /* Standard Mode */
#define I2C_SCL_SPEED_FM           400000  /* Fast Mode */



#define I2C_NOSTRETCH_ENABLE   1
#define I2C_NOSTRETCH_DISABLE  0

#define I2C_ERROR_NONE      0
#define I2C_ERROR_NACK      1
#define I2C_ERROR_TIMEOUT   2

void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_RegDef_t *pI2Cx);


int I2C_MemRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint8_t Reg, uint8_t *pBuf, uint32_t Len);



uint8_t I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr);

uint8_t I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr);

/*
 *IRQ Configuration and ISR handling
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);


/*
*	Other Peripheral Control APIs
*/

void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);

void I2C_ApplicationEventCallBack(I2C_Handle_t *pI2CHandle, uint8_t AppEv);


#endif /* INC_STM32L47XX_I2C_DRIVER_H_ */
