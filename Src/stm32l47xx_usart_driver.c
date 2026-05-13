/*
 * stm32l47xx_usart_driver.c
 *
 *  Created on: May 30, 2025
 *      Author: admin
 */

#include "stm32l47xx_usart_driver.h"
#include <stddef.h>

/*
 * Computes the USART BRR value for a given peripheral clock and baud rate.
 *
 * This implementation assumes oversampling by 16, which is the default mode
 * when the OVER8 bit in USART_CR1 is cleared.
 *
 * For oversampling by 16:
 *
 *     BRR = USART peripheral clock / baud rate
 *
 * Rounded division is used to reduce baud rate error.
 */
static uint32_t USART_ComputeBRR(uint32_t usart_clk, uint32_t baudrate)
{
    if (baudrate == 0U)
    {
        return 0U;
    }

    return (usart_clk + (baudrate / 2U)) / baudrate;
}

/*
 * Returns the clock frequency used by the USART peripheral.
 *
 * Current implementation:
 * - The driver assumes that the USART kernel clock is HSI16.
 * - HSI16 runs at 16 MHz on STM32L4.
 *
 * Future improvement:
 * - Read RCC->CCIPR to detect whether the USART clock source is PCLK,
 *   SYSCLK, HSI16, or LSE.
 * - Return the actual selected clock frequency.
 */
static uint32_t USART_GetClockValue(USART_RegDef_t *pUSARTx)
{
    (void)pUSARTx;

    return USART_CLK_HSI16;
}

/*
 * Configures the USART baud rate.
 *
 * The USART must be disabled before writing BRR if the baud rate is changed
 * during runtime. During USART_Init(), this function is called while UE = 0.
 *
 * Current limitation:
 * - Only oversampling by 16 is supported.
 * - OVER8 must remain cleared.
 */
void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t baudrate)
{
    uint32_t usart_clk = USART_GetClockValue(pUSARTx);

    pUSARTx->BRR = USART_ComputeBRR(usart_clk, baudrate);
}


/*
 * Peripheral Clock setup
 */
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
		{
			if(pUSARTx==USART1)
			{
				USART1_PCLK_EN();
			}else if (pUSARTx==USART2)
			{
				USART2_PCLK_EN();
			}else if (pUSARTx==USART3)
			{
				USART1_PCLK_EN();
			}else if (pUSARTx==UART4)
			{
				UART4_PCLK_EN();
			}else if (pUSARTx==UART5)
			{
				UART5_PCLK_EN();
			}
		}else if(EnorDi==DISABLE)
		{
			if(pUSARTx==USART1)
			{
				USART1_PCLK_DI();
			}else if (pUSARTx==USART2)
			{
				USART2_PCLK_DI();
			}else if (pUSARTx==USART3)
			{
				USART1_PCLK_DI();
			}else if (pUSARTx==UART4)
			{
				UART4_PCLK_DI();
			}else if (pUSARTx==UART5)
			{
				UART5_PCLK_DI();
			}
		}
}

/*
 * Init and De-init
 */
void USART_Init(USART_Handle_t *pUSARTHandle)

	/*********************************************************************
	 * @fn      		  - USART_Init
	 *
	 * @brief             -
	 *
	 * @param[in]         -
	 * @param[in]         -
	 * @param[in]         -
	 *
	 * @return            -
	 *
	 * @Note              -

	 */
	{

		//Temporary variable
		uint32_t tempreg=0;



	/******************************** Configuration of CR1******************************************/

		//Implement the code to enable the Clock for given USART peripheral
		USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);

		pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_UE);
		//Enable USART Tx and Rx engines according to the USART_Mode configuration item
		if ( pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
		{
			//Implement the code to enable the Receiver bit field
			tempreg|= (1 << USART_CR1_RE);
		}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
		{
			//Implement the code to enable the Transmitter bit field
			tempreg |= ( 1 << USART_CR1_TE );

		}else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
		{
			//Implement the code to enable the both Transmitter and Receiver bit fields
			tempreg |= ( ( 1 << USART_CR1_TE) | ( 1 << USART_CR1_RE) );
		}

	    //Implement the code to configure the Word length configuration item
		tempreg |= pUSARTHandle->USART_Config.USART_WordLength << USART_CR1_M0 ;


	    //Configuration of parity control bit fields
		if ( pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
		{
			//Implement the code to enable the parity control
			tempreg |= ( 1 << USART_CR1_PCE);

			//Implement the code to enable EVEN parity
			//Not required because by default EVEN parity will be selected once you enable the parity control

		}else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD )
		{
			//Implement the code to enable the parity control
		    tempreg |= ( 1 << USART_CR1_PCE);

		    //Implement the code to enable ODD parity
		    tempreg |= ( 1 << USART_CR1_PS);

		}
		/*
		 * In STM32 USART, the baud rate generator depends on the oversampling mode.
		 * Keeping OVER8 cleared makes the BRR calculation straightforward:
		 *
		 *     BRR = USART clock / baud rate
		 *
		 * Oversampling by 8 can be added later, but it requires a different BRR
		 * encoding.
		 */
			tempreg &= ~( 1 << USART_CR1_OVER8);

	   //Program the CR1 register
		pUSARTHandle->pUSARTx->CR1 = tempreg;


	/******************************** Configuration of CR2******************************************/

		tempreg=0;

		//Implement the code to configure the number of stop bits inserted during USART frame transmission
		tempreg |= pUSARTHandle->USART_Config.USART_NoOfStopBits << USART_CR2_STOP;

		//Program the CR2 register
		pUSARTHandle->pUSARTx->CR2= tempreg;

	/******************************** Configuration of CR3******************************************/

		tempreg=0;

		//Configuration of USART hardware flow control
		if ( pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
		{
			//Implement the code to enable CTS flow control
			tempreg |= ( 1 << USART_CR3_CTSE);


		}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
		{
			//Implement the code to enable RTS flow control
			tempreg |= (1 << USART_CR3_RTSE);

		}else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
		{
			//Implement the code to enable both CTS and RTS Flow control
			tempreg |= ((1 << USART_CR3_CTSE) | (1 << USART_CR3_RTSE));
		}


		pUSARTHandle->pUSARTx->CR3 = tempreg;

	/******************************** Configuration of BRR(Baudrate register)******************************************/

		//Implement the code to configure the baud rate


		USART_SetBaudRate(pUSARTHandle->pUSARTx, pUSARTHandle->USART_Config.USART_Baud);

		/*************************** Enable the peripheral*************************************************************/
		pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_UE);

	}


void USART_DeInit(USART_RegDef_t *pUSARTx)
{
	if(pUSARTx==USART1)
				{
					USART1_REG_RESET();
				}else if (pUSARTx==USART2)
				{
					USART2_REG_RESET();
				}else if (pUSARTx==USART3)
				{
					USART3_REG_RESET();
				}else if (pUSARTx==UART4)
				{
					UART4_REG_RESET();
				}else if (pUSARTx==UART5)
				{
					UART5_REG_RESET();
				}
}


uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName)
{
	if (pUSARTx->ISR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*
 * Data Send and Receive
 */
/*********************************************************************
 * @fn      		  - USART_SendData
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              - Resolve all the TODOs

 */
uint8_t USART_SendData(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len)
{

	uint32_t timeout;

	    if ((pUSARTHandle == NULL) || (pTxBuffer == NULL))
	    {
	        return USART_ERROR;
	    }

	    if (Len == 0U)
	    {
	        return USART_OK;
	    }

	    for (uint32_t i = 0; i < Len; i++)
	    {
	        timeout = USART_TIMEOUT_COUNT;

	        /*
	         * Wait until the transmit data register is empty.
	         * TXE/TXFNF means the USART is ready to accept a new byte in TDR.
	         */
	        while (!(pUSARTHandle->pUSARTx->ISR & (1U << USART_ISR_TXE)))
	        {
	            if (timeout-- == 0U)
	            {
	                return USART_TIMEOUT;
	            }
	        }

	        /*
	         * This driver currently supports 8-bit data transfers.
	         * Only the lower 8 bits are written to TDR.
	         */
	        pUSARTHandle->pUSARTx->TDR = (*pTxBuffer & 0xFFU);
	        pTxBuffer++;
	    }

	    timeout = USART_TIMEOUT_COUNT;

	    /*
	     * Wait until the last byte has completely shifted out.
	     * TC is different from TXE: TXE only means TDR is empty, while TC means
	     * the full frame, including stop bit, has been transmitted.
	     */
	    while (!(pUSARTHandle->pUSARTx->ISR & (1U << USART_ISR_TC)))
	    {
	        if (timeout-- == 0U)
	        {
	            return USART_TIMEOUT;
	        }
	    }

	    return USART_OK;
}


/*********************************************************************
 * @fn      		  - USART_ReceiveData
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */


uint8_t USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	uint32_t timeout;

	    if ((pUSARTHandle == NULL) || (pRxBuffer == NULL))
	    {
	        return USART_ERROR;
	    }

	    for (uint32_t i = 0; i < Len; i++)
	    {
	        timeout = USART_TIMEOUT_COUNT;

	        /*
	         * Wait until a byte is available in the receive data register.
	         * RXNE/RXFNE means that RDR contains unread received data.
	         */
	        while (!(pUSARTHandle->pUSARTx->ISR & (1U << USART_ISR_RXNE)))
	        {
	            if (timeout-- == 0U)
	            {
	                return USART_TIMEOUT;
	            }
	        }

	        /*
	         * This driver currently supports 8-bit data transfers.
	         * Reading RDR clears the RXNE/RXFNE flag.
	         */
	        pRxBuffer[i] = (uint8_t)(pUSARTHandle->pUSARTx->RDR & 0xFFU);
	    }

	    return USART_OK;

}

/*********************************************************************
 * @fn      		  - USART_SendDataWithIT
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              - Resolve all the TODOs

 */

/*
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t txstate = pUSARTHandle->TODO;

	if(txstate != USART_BUSY_IN_TX)
	{
		pUSARTHandle->TODO = Len;
		pUSARTHandle->pTxBuffer = TODO;
		pUSARTHandle->TxBusyState = TODO;

		//Implement the code to enable interrupt for TXE
		TODO


		//Implement the code to enable interrupt for TC
		TODO


	}

	return txstate;

}


/*********************************************************************
 * @fn      		  - USART_ReceiveDataIT
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              - Resolve all the TODOs

 */
/*
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle,uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t rxstate = pUSARTHandle->TODO;

	if(rxstate != TODO)
	{
		pUSARTHandle->RxLen = Len;
		pUSARTHandle->pRxBuffer = TODO;
		pUSARTHandle->RxBusyState = TODO;

		//Implement the code to enable interrupt for RXNE
		TODO

	}

	return rxstate;

}
*/

/*
 * IRQ Configuration and ISR handling
 */
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ISER0 |= (1 << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
		}
	}
	else
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ICER0 |= (1 << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
		}
	}
}

void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

void USART_EnableRXNEInterrupt(USART_Handle_t *pUSARTHandle)
{
	pUSARTHandle->pUSARTx->CR1 |= (1U << USART_CR1_RXNEIE);
}

void USART_IRQHandling(USART_Handle_t *pHandle)
{
	if ((pHandle->pUSARTx->ISR & (1U << USART_ISR_RXNE)) &&
	    (pHandle->pUSARTx->CR1 & (1U << USART_CR1_RXNEIE)))
	{
		pHandle->RxByte = (uint8_t)(pHandle->pUSARTx->RDR & 0xFFU);
		USART_ApplicationEventCallback(pHandle, USART_EVENT_RXNE);
	}
}

/*
 * Other Peripheral Control APIs
 */
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnOrDi){
	if (EnOrDi == ENABLE)
		{
			pUSARTx->CR1 |= (1 << USART_CR1_UE);
		}else
		{
			pUSARTx->CR1 &= ~(1 << USART_CR1_UE);
		}
}
//uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx , uint32_t FlagName);
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t StatusFlagName);

/*
 * Application callback
 */
__attribute__((weak)) void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle,uint8_t AppEv)
{
	(void)pUSARTHandle;
	(void)AppEv;
}
