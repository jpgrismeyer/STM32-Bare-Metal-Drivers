#include "stm32l47xx.h"
#include <string.h>

#define SPI_CS_PORT        GPIOA
#define SPI_CS_PIN         GPIO_PIN_NO_2

static void delay(void)
{
	for (volatile uint32_t i = 0; i < 300000U; i++);
}

// Initialize SPI1 pins available on the Arduino connector.
// Arduino D13 -> PA5 -> SPI1_SCK
// Arduino D11 -> PA7 -> SPI1_MOSI
// Arduino D10 -> PA2 -> software chip select marker
void SPI1_GPIOInits(void)
{
	GPIO_Handle_t SPIPins;

	SPIPins.pGPIOx = GPIOA;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	// SPI1 SCK on Arduino D13
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GPIO_Init(&SPIPins);

	// SPI1 MOSI on Arduino D11
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&SPIPins);
}

// Use PA2 / Arduino D10 as a manual chip-select marker for the logic analyzer.
void SPI1_CS_GPIOInit(void)
{
	GPIO_Handle_t CSPin;

	CSPin.pGPIOx = SPI_CS_PORT;
	CSPin.GPIO_PinConfig.GPIO_PinNumber = SPI_CS_PIN;
	CSPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	CSPin.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	CSPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	CSPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&CSPin);
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, ENABLE);
}

void SPI1_Inits(void)
{
	SPI_Handle_t SPI1Handle;

	SPI1Handle.pSPIx = SPI1;
	SPI1Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI1Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI1Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV64;
	SPI1Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI1Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	SPI1Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI1Handle.SPIConfig.SPI_SSM = SPI_SSM_EN;

	SPI_Init(&SPI1Handle);
	SPI_SSIConfig(SPI1, ENABLE);
}

static void SPI1_SendFrame(uint8_t *data, uint32_t len)
{
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, DISABLE);

	SPI_PeripheralControl(SPI1, ENABLE);
	if (SPI_SendDataWithStatus(SPI1, data, len) == SPI_OK)
	{
		while (SPI_GetFlagStatus(SPI1, SPI_BUSY_FLAG) == FLAG_SET);
	}

	SPI_PeripheralControl(SPI1, DISABLE);
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, ENABLE);
}

int main(void)
{
	uint8_t text[] = "SPI1 ARD D13 SCK D11 MOSI";
	uint8_t pattern[] = {0x55, 0xA5, 0xF0, 0x0F};

	SystemClock_HSI_Init();

	SPI1_GPIOInits();
	SPI1_CS_GPIOInit();
	SPI1_Inits();

	while (1)
	{
		SPI1_SendFrame(text, strlen((char *)text));
		delay();

		SPI1_SendFrame(pattern, sizeof(pattern));
		delay();
	}
}
