#include "stm32l47xx.h"
#include "stm32l475xx_spi_driver.h"
#include <stddef.h>

void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (pSPIx == SPI1)
		{
			SPI1_PCLK_EN();
		}
		else if (pSPIx == SPI2)
		{
			SPI2_PCLK_EN();
		}
		else if (pSPIx == SPI3)
		{
			SPI3_PCLK_EN();
		}
	}
	else
	{
		if (pSPIx == SPI1)
		{
			SPI1_PCLK_DI();
		}
		else if (pSPIx == SPI2)
		{
			SPI2_PCLK_DI();
		}
		else if (pSPIx == SPI3)
		{
			SPI3_PCLK_DI();
		}
	}
}

void SPI_Init(SPI_Handle_t *pSPIHandle)
{
	if ((pSPIHandle == NULL) || (pSPIHandle->pSPIx == NULL))
	{
		return;
	}

	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	uint32_t tempreg = 0;
	uint32_t tempcr2 = 0;

	tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

	if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		tempreg &= ~(1U << SPI_CR1_BIDI_MODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		tempreg |= (1U << SPI_CR1_BIDI_MODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		tempreg &= ~(1U << SPI_CR1_BIDI_MODE);
		tempreg |= (1U << SPI_CR1_RX_ONLY);
	}

	tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;
	tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;
	tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;
	tempreg |= pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM;

	/*
	 * STM32L4 selects SPI data size through CR2.DS.
	 * DS = 0b0111 means 8-bit frames, DS = 0b1111 means 16-bit frames.
	 */
	if (pSPIHandle->SPIConfig.SPI_DFF == SPI_DFF_16BITS)
	{
		tempcr2 |= (0xFU << SPI_CR2_DS);
	}
	else
	{
		tempcr2 |= (0x7U << SPI_CR2_DS);
		tempcr2 |= (1U << SPI_CR2_FRXTH);
	}

	pSPIHandle->pSPIx->CR1 = tempreg;
	pSPIHandle->pSPIx->CR2 = tempcr2;
}

void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	if (pSPIx == SPI1)
	{
		SPI1_REG_RESET();
	}
	else if (pSPIx == SPI2)
	{
		SPI2_REG_RESET();
	}
	else if (pSPIx == SPI3)
	{
		SPI3_REG_RESET();
	}
}

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
	if (pSPIx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	(void)SPI_SendDataWithStatus(pSPIx, pTxBuffer, Len);
}

uint8_t SPI_SendDataWithStatus(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	if ((pSPIx == NULL) || (pTxBuffer == NULL))
	{
		return SPI_ERROR;
	}

	while (Len > 0)
	{
		uint32_t timeout = SPI_TIMEOUT_COUNT;

		while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET)
		{
			if (timeout-- == 0U)
			{
				return SPI_TIMEOUT;
			}
		}

		if (((pSPIx->CR2 >> SPI_CR2_DS) & 0xFU) > 0x7U)
		{
			if (Len < 2U)
			{
				return SPI_ERROR;
			}

			pSPIx->DR = *((uint16_t*)pTxBuffer);
			Len -= 2U;
			pTxBuffer += 2U;
		}
		else
		{
			*((__vo uint8_t *)&pSPIx->DR) = *pTxBuffer;
			Len--;
			pTxBuffer++;
		}
	}

	return SPI_OK;
}

void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	(void)SPI_ReceiveDataWithStatus(pSPIx, pRxBuffer, Len);
}

uint8_t SPI_ReceiveDataWithStatus(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	if ((pSPIx == NULL) || (pRxBuffer == NULL))
	{
		return SPI_ERROR;
	}

	while (Len > 0)
	{
		uint32_t timeout = SPI_TIMEOUT_COUNT;

		while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET)
		{
			if (timeout-- == 0U)
			{
				return SPI_TIMEOUT;
			}
		}

		if (((pSPIx->CR2 >> SPI_CR2_DS) & 0xFU) > 0x7U)
		{
			if (Len < 2U)
			{
				return SPI_ERROR;
			}

			*((uint16_t*)pRxBuffer) = (uint16_t)pSPIx->DR;
			Len -= 2U;
			pRxBuffer += 2U;
		}
		else
		{
			*pRxBuffer = *((__vo uint8_t *)&pSPIx->DR);
			Len--;
			pRxBuffer++;
		}
	}

	return SPI_OK;
}

void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
	{
		pSPIx->CR1 |= (1U << SPI_CR1_SPE);
	}
	else
	{
		pSPIx->CR1 &= ~(1U << SPI_CR1_SPE);
	}
}

void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
	{
		pSPIx->CR1 |= (1U << SPI_CR1_SSI);
	}
	else
	{
		pSPIx->CR1 &= ~(1U << SPI_CR1_SSI);
	}
}

void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
	{
		pSPIx->CR2 |= (1U << SPI_CR2_SSOE);
	}
	else
	{
		pSPIx->CR2 &= ~(1U << SPI_CR2_SSOE);
	}
}

/*
 * Writes one data unit (8 or 16 bits, same DS-register check the blocking
 * functions use) to DR and advances the buffer/length. Shared by the IT
 * send path and by the IRQ handler's TXE branch, so there's exactly one
 * place that knows how to push a unit into DR.
 */
static void SPI_TXE_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
	if (((pSPIHandle->pSPIx->CR2 >> SPI_CR2_DS) & 0xFU) > 0x7U)
	{
		pSPIHandle->pSPIx->DR = *((uint16_t *)pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen -= 2U;
		(pSPIHandle->pTxBuffer) += 2U;
	}
	else
	{
		*((__vo uint8_t *)&pSPIHandle->pSPIx->DR) = *(pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen--;
		(pSPIHandle->pTxBuffer)++;
	}

	if (pSPIHandle->TxLen == 0U)
	{
		/* Nothing left to send -- stop caring about TXE and hand back to the app. */
		SPI_CloseTransmission(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
	}
}

static void SPI_RXNE_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
	if (((pSPIHandle->pSPIx->CR2 >> SPI_CR2_DS) & 0xFU) > 0x7U)
	{
		*((uint16_t *)pSPIHandle->pRxBuffer) = (uint16_t)pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen -= 2U;
		(pSPIHandle->pRxBuffer) += 2U;
	}
	else
	{
		*(pSPIHandle->pRxBuffer) = *((__vo uint8_t *)&pSPIHandle->pSPIx->DR);
		pSPIHandle->RxLen--;
		(pSPIHandle->pRxBuffer)++;
	}

	if (pSPIHandle->RxLen == 0U)
	{
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
	}
}

/*
 * OVR (overrun) is only meaningful mid-reception -- a byte arrived while the
 * previous one hadn't been read from DR yet. Cleared by the read-DR-then-SR
 * sequence the reference manual specifies. Reported to the app either way,
 * since it means data was lost.
 */
static void SPI_OVR_ERR_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
	if (pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		SPI_ClearOVRFlag(pSPIHandle->pSPIx);
	}

	SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	uint32_t temp;

	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}

uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
	if ((pSPIHandle == NULL) || (pTxBuffer == NULL) || (Len == 0U))
	{
		return SPI_ERROR;
	}

	if (pSPIHandle->TxState == SPI_BUSY_IN_TX)
	{
		return SPI_BUSY_IN_TX;
	}

	pSPIHandle->pTxBuffer = pTxBuffer;
	pSPIHandle->TxLen = Len;
	pSPIHandle->TxState = SPI_BUSY_IN_TX;

	pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_TXEIE) | (1U << SPI_CR2_ERRIE);

	return SPI_OK;
}

uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	if ((pSPIHandle == NULL) || (pRxBuffer == NULL) || (Len == 0U))
	{
		return SPI_ERROR;
	}

	if (pSPIHandle->RxState == SPI_BUSY_IN_RX)
	{
		return SPI_BUSY_IN_RX;
	}

	pSPIHandle->pRxBuffer = pRxBuffer;
	pSPIHandle->RxLen = Len;
	pSPIHandle->RxState = SPI_BUSY_IN_RX;

	pSPIHandle->pSPIx->CR2 |= (1U << SPI_CR2_RXNEIE) | (1U << SPI_CR2_ERRIE);

	return SPI_OK;
}

void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0U;
	pSPIHandle->TxState = SPI_READY;
}

void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~(1U << SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0U;
	pSPIHandle->RxState = SPI_READY;
}

__attribute__((weak)) void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{
	(void)pSPIHandle;
	(void)AppEv;
	/* Weak default so the driver links before an app overrides this. */
}

void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
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

void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

/*
 * Single entry point for both the TXE and RXNE cases -- SPI1/2/3 share one
 * IRQ line for both events (unlike I2C's separate EV/ER lines), so we check
 * each source flag against its corresponding IE bit in turn.
 */
void SPI_IRQHandling(SPI_Handle_t *pHandle)
{
	uint8_t temp1, temp2;

	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_TXE);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_TXEIE);
	if (temp1 && temp2)
	{
		SPI_TXE_Interrupt_Handle(pHandle);
	}

	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_RXNE);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_RXNEIE);
	if (temp1 && temp2)
	{
		SPI_RXNE_Interrupt_Handle(pHandle);
	}

	temp1 = pHandle->pSPIx->SR & (1U << SPI_SR_OVR);
	temp2 = pHandle->pSPIx->CR2 & (1U << SPI_CR2_ERRIE);
	if (temp1 && temp2)
	{
		SPI_OVR_ERR_Interrupt_Handle(pHandle);
	}
}
