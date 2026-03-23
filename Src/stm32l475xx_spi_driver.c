#include "stm32l47xx.h"
#include "stm32l475xx_spi_driver.h"


void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(pSPIx==SPI1)
		{
			SPI1_PCLK_EN();
		}else if (pSPIx==SPI2)
		{
			SPI2_PCLK_EN();
		}else if (pSPIx==SPI3)
		{
			SPI3_PCLK_EN();
		}
	}
		else if(EnorDi==DISABLE)
			{
				if(pSPIx==SPI1)
				{
					SPI1_PCLK_DI();
				}else if (pSPIx==SPI2)
				{
					SPI2_PCLK_DI();
				}else if (pSPIx==SPI3)
				{
					SPI3_PCLK_DI();
				}
			}
}


void SPI_Init(SPI_Handle_t *pSPIHandle)
{
	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	uint32_t tempreg = 0;

	// 1. Modo maestro/esclavo
	tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

	// 2. Configuración del tipo de bus
	if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		tempreg &= ~(1 << SPI_CR1_BIDI_MODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		tempreg |= (1 << SPI_CR1_BIDI_MODE);
	}
	else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		tempreg &= ~(1 << SPI_CR1_BIDI_MODE);
		tempreg |= (1 << SPI_CR1_RX_ONLY);
	}

	// 3. Resto de la configuración (siempre necesaria)
	tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;
	tempreg |= pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_CRCL;
	tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;
	tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;
	tempreg |= pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM;

	// 4. Escritura final del CR1
	pSPIHandle->pSPIx->CR1 = tempreg;
}

void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	if(pSPIx==SPI1)
			{
				SPI1_REG_RESET();
			}else if (pSPIx==SPI2)
			{
				SPI2_REG_RESET();
			}else if (pSPIx==SPI3)
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

void SPI_SendData (SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	while (Len > 0)
		{
			// Esperar a que el buffer esté vacío
			while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET);

			if (pSPIx->CR1 & (1 << SPI_CR1_CRCL)) // 16 bits
			{
				// casteás el buffer a 16 bits y escribís al registro
				pSPIx->DR = *((uint16_t*)pTxBuffer);
				Len -= 2;
				(uint16_t*)pTxBuffer++;
			}
			else // 8 bits
			{
				*((__vo uint8_t *)&pSPIx->DR) = *pTxBuffer;     //se usa & porque vamos a escribir solo 1 byte
				Len--;
				pTxBuffer++;
			}
		}
	}

void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	while (Len > 0)
	{
		// Esperar a que el buffer de recepción tenga datos
		while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET);

		if (pSPIx->CR1 & (1 << SPI_CR1_CRCL)) // 16 bits
		{
			*((uint16_t*)pRxBuffer) = (uint16_t)pSPIx->DR;
			Len -= 2;
			pRxBuffer += 2;
		}
		else // 8 bits
		{
			*pRxBuffer = *((__vo uint8_t *)&pSPIx->DR);
			Len--;
			pRxBuffer++;
		}
	}
}



void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
	{
		pSPIx->CR1 |= (1 << SPI_CR1_SPE);
	}else
	{
		pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
		{
			pSPIx->CR1 |= (1 << SPI_CR1_SSI);

		}else
		{
			pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
		}
}

/*void SPI_SSMConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
		pSPIx->CR1 |= (1 << SPI_CR1_SSM);
	else
		pSPIx->CR1 &= ~(1 << SPI_CR1_SSM);
}*/

void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnOrDi)
{
	if (EnOrDi == ENABLE)
		{
			pSPIx->CR2 |= (1 << SPI_CR2_SSOE);

		}else
		{
			pSPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
		}
}
