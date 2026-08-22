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
	uint8_t  I2C_AutoEnd;
} I2C_Config_t;

typedef struct
{
    I2C_RegDef_t *pI2Cx;
    I2C_Config_t I2CConfig;

	/* IT (interrupt-driven) transfer state. Blocking I2C_MasterSendData/ReceiveData
	 * don't touch these -- only the *_IT variants and the IRQ handlers do. */
	uint8_t  *pTxBuffer;
	uint8_t  *pRxBuffer;
	uint32_t TxLen;
	uint32_t RxLen;
	uint8_t  TxRxState;   /* I2C_READY / I2C_BUSY_IN_TX / I2C_BUSY_IN_RX */
	uint8_t  DevAddr;     /* slave address of the transfer currently in flight */
	uint8_t  Sr;          /* I2C_SR_ENABLE/DISABLE for the transfer currently in flight */
} I2C_Handle_t;


/*
 * I2C Timing values para I2CCLK = 16MHz
 * Estos valores están calculados para cumplir con los tiempos de subida/bajada de la norma I2C
 */
#define I2C_TIMING_5KHZ_16MHZ	   0x309095FF
#define I2C_TIMING_100KHZ_16MHZ    0x00303D5B
#define I2C_TIMING_400KHZ_16MHZ    0x0010061A

/* Macros para que el usuario elija en el main */
#define I2C_SCL_SPEED_5KHZ      5000		/*to test when noisy*/
#define I2C_SCL_SPEED_SM           100000  /* Standard Mode */
#define I2C_SCL_SPEED_FM           400000  /* Fast Mode */



#define I2C_NOSTRETCH_ENABLE   1
#define I2C_NOSTRETCH_DISABLE  0

#define I2C_ERROR_NONE      0
#define I2C_ERROR_NACK      -1
#define I2C_ERROR_TIMEOUT   -2

#define I2C_AUTOEND_DISABLE 0
#define I2C_AUTOEND_ENABLE 1

#define I2C_SR_DISABLE 		0
#define I2C_SR_ENABLE 		1

/* @I2C_TxRxState -- pI2CHandle->TxRxState */
#define I2C_READY           0
#define I2C_BUSY_IN_TX       1
#define I2C_BUSY_IN_RX       2

/* @I2C_ApplicationEvents -- second argument passed to I2C_ApplicationEventCallBack() */
#define I2C_EVENT_TX_CMPLT      1   /* IT send finished (STOP generated, or bus held for repeated START) */
#define I2C_EVENT_RX_CMPLT      2   /* IT receive finished */
#define I2C_ERROR_EVENT_NACK    3   /* slave (or a data byte) was not acknowledged */
#define I2C_ERROR_EVENT_BERR    4   /* bus error: misplaced START/STOP */
#define I2C_ERROR_EVENT_ARLO    5   /* arbitration lost (another master won the bus) */
#define I2C_ERROR_EVENT_OVR     6   /* overrun/underrun on the data register */

/* Returned by the *_IT functions when a transfer is already in flight on this handle. */
#define I2C_ERROR_BUSY      (-3)

void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi);

void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_RegDef_t *pI2Cx);


//int I2C_MemRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint8_t Reg, uint8_t *pBuf, uint32_t Len);



uint8_t I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

uint8_t I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/*
 * Interrupt-driven (non-blocking) master transfers. Kick off the transfer and return
 * immediately -- I2C_READY_check via pI2CHandle->TxRxState, or just wait for
 * I2C_ApplicationEventCallBack() to fire with I2C_EVENT_TX_CMPLT / RX_CMPLT / one of the
 * I2C_ERROR_EVENT_* codes. Requires I2C_IRQInterruptConfig()+I2C_IRQPriorityConfig() to
 * have been called for both the EV and ER IRQ lines of this peripheral, and
 * I2C_EV_IRQHandling()/I2C_ER_IRQHandling() to be wired up from the two ISRs.
 *
 * Returns I2C_ERROR_BUSY if a previous IT transfer on this handle hasn't completed yet.
 */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr);

/*
 *IRQ Configuration and ISR handling
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);

/* Call from I2Cx_EV_IRQHandler() / I2Cx_ER_IRQHandler() respectively. */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);

/* Force-abort an in-flight IT transfer: disables the interrupt-enable bits this driver
 * turned on and resets TxRxState back to I2C_READY. Called internally once a transfer
 * finishes (successfully or not) -- exposed publicly too, for an app-level watchdog. */
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);

/*
*	Other Peripheral Control APIs
*/

void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint8_t FlagName);

void I2C_ApplicationEventCallBack(I2C_Handle_t *pI2CHandle, uint8_t AppEv);


#endif /* INC_STM32L47XX_I2C_DRIVER_H_ */
