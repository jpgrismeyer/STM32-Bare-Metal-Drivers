/*
 * stm32l475xx_spi.h
 *
 *  Created on: Mar 17, 2025
 *      Author: admin
 */

#ifndef INC_STM32L475XX_SPI_DRIVER_H_
#define INC_STM32L475XX_SPI_DRIVER_H_

#include"stm32l47xx.h"

/*
 * Configuration structure for SPIx peripheral
 */


typedef struct
{
	uint8_t SPI_DeviceMode;
	uint8_t SPI_BusConfig;
	uint8_t SPI_SclkSpeed;
	uint8_t SPI_DFF;
	uint8_t SPI_CPOL;
	uint8_t SPI_CPHA;
	uint8_t SPI_SSM;
}SPI_Config_t;

/*
 * This is the handle structure for a SPIx peripheral
 */

typedef struct
{
	SPI_RegDef_t *pSPIx;		/*Holds the base address of SPIx (x: 0,1,2) peripheral*/
	SPI_Config_t SPIConfig;

	/* IT transfer state -- only touched by SPI_SendDataIT/SPI_ReceiveDataIT and the IRQ handler. */
	uint8_t  *pTxBuffer;
	uint8_t  *pRxBuffer;
	uint32_t TxLen;
	uint32_t RxLen;
	uint8_t  TxState;
	uint8_t  RxState;
}SPI_Handle_t;

/*
 * @SPI_DeviceMode
 */
#define SPI_DEVICE_MODE_MASTER 	1
#define SPI_DEVICE_MODE_SLAVE 	0

/*
 * @SPI_BusConfig
 */
#define SPI_BUS_CONFIG_FD 					1
#define SPI_BUS_CONFIG_HD 					2
#define SPI_BUS_CONFIG_SIMPLEX_TXONLY 		3
#define SPI_BUS_CONFIG_SIMPLEX_RXONLY 		4

/*
 * @SPI_SclkSpeed
 * The SPI sclck speed is the result of the bus speed divided by a preescaler
 */

#define SPI_SCLK_SPEED_DIV2 				0
#define SPI_SCLK_SPEED_DIV4 				1
#define SPI_SCLK_SPEED_DIV8 				2
#define SPI_SCLK_SPEED_DIV16 				3
#define SPI_SCLK_SPEED_DIV32 				4
#define SPI_SCLK_SPEED_DIV64 				5
#define SPI_SCLK_SPEED_DIV128 				6
#define SPI_SCLK_SPEED_DIV256 				7

/*
 * @SPI_DFF
 */

#define SPI_DFF_8BITS 						0
#define SPI_DFF_16BITS 						1

/*
 * @SPI_CPOL
 */

#define SPI_CPOL_LOW 						0
#define SPI_CPOL_HIGH 						1

/*
 * @SPI_CPHA
 */

#define SPI_CPHA_LOW 						0
#define SPI_CPHA_HIGH 						1

/*
 * @SPI_SSM
 */

#define SPI_SSM_DI 						0
#define SPI_SSM_EN 						1

/*
 * SPI related status flags definitions (MASKING DETAILS FOR SR REGISTER)
 */
#define SPI_TXE_FLAG		(1 << SPI_SR_TXE)
#define SPI_RXNE_FLAG		(1 << SPI_SR_RXNE)
#define SPI_BUSY_FLAG		(1 << SPI_SR_BSY)

#define SPI_OK              0
#define SPI_TIMEOUT         1
#define SPI_ERROR           2

#define SPI_TIMEOUT_COUNT   100000U

/*
 * @SPI_TxRxState -- IT transfer state (SPI_Handle_t.TxState / RxState)
 */
#define SPI_READY			0
#define SPI_BUSY_IN_TX		1
#define SPI_BUSY_IN_RX		2

/*
 * @SPI_ApplicationEvent -- codes passed to SPI_ApplicationEventCallback()
 */
#define SPI_EVENT_TX_CMPLT	1
#define SPI_EVENT_RX_CMPLT	2
#define SPI_EVENT_OVR_ERR	3
/*Peripheral Clock setup
 *
 */

void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi);

/*
 *Init and De-init
 */
void SPI_Init(SPI_Handle_t *pSPIHandle);
void SPI_DeInit(SPI_RegDef_t *pSPIx);

/*
 *Data Send and Receive
 */

void SPI_SendData (SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len);
void SPI_ReceiveData (SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len);
uint8_t SPI_SendDataWithStatus(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len);
uint8_t SPI_ReceiveDataWithStatus(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len);
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName);

/*
 * Non-blocking (interrupt-driven) send/receive. Kick off the transfer and
 * return immediately; SPI_IRQHandling() does the rest, byte by byte, from
 * the ISR. Returns SPI_BUSY_IN_TX/SPI_BUSY_IN_RX if a transfer of that kind
 * is already in flight (won't clobber it), SPI_OK otherwise.
 */
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len);
uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len);
void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle);
void SPI_CloseReception(SPI_Handle_t *pSPIHandle);
void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx);

/*
 *IRQ Configuration and ISR handling
 */
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);
void SPI_IRQHandling(SPI_Handle_t *pHandle);

/*
 * Weak default so the driver links even before an application overrides it
 * (same pattern as I2C_ApplicationEventCallBack / USART_ApplicationEventCallback).
 */
void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv);

/*
*	Other Peripheral Control APIs
*/

void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi);
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi);
//void SPI_SSMConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi);
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi);


#endif /* INC_STM32L475XX_SPI_DRIVER_H_ */
