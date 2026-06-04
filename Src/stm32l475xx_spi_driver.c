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
