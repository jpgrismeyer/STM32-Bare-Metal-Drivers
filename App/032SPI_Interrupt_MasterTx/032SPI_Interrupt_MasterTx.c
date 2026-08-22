/*
 * 032SPI_Interrupt_MasterTx.c
 *
 * SPI master transmit done entirely through SPI_SendDataIT() -- no blocking while
 * loop anywhere in the transfer path. Same evolution the I2C driver went through in
 * 031I2C_Interrupt_MasterTxRx: the peripheral was polling-only, now a command kicks
 * off a transfer and returns immediately, and SPI_IRQHandling() (fed from SPI1_IRQHandler)
 * does the rest byte by byte from the ISR.
 *
 * Target: an external MFRC522 (RC522) RFID reader module wired to SPI1, chip-select on
 * a plain GPIO (same manual-CS pattern the SD-over-SPI driver already uses in App 030 --
 * the RC522's CS timing doesn't need anything hardware NSS/SSOE would give us either).
 *
 * MFRC522 SPI framing: address byte = (register << 1) & 0x7E, MSB clear for a write.
 * SPI RESET sends [0x02, 0x0F] -- write CommandReg (0x01) = SoftReset (0x0F), a safe,
 * idempotent command any RC522 will accept regardless of its prior state. SPI SEND
 * takes an arbitrary space-separated hex byte list for anything else worth trying.
 *
 * PA5 -> SPI1_SCK, PA6 -> SPI1_MISO (unused here, wired for completeness), PA7 -> SPI1_MOSI,
 * PA4 -> manual CS (active low). PB6/PB7 -> USART1 console, same as every other app in
 * this repo.
 */

#include "stm32l47xx.h"
#include "ring_buffer.h"
#include <string.h>

#define CMD_BUFFER_SIZE         64
#define RX_BUFFER_SIZE          128
#define MAX_SPI_TX_BYTES        8

#define SPI_CS_PORT              GPIOA
#define SPI_CS_PIN               GPIO_PIN_NO_4

USART_Handle_t USART1Handle;
SPI_Handle_t SPI1Handle;

static volatile uint8_t rx_storage[RX_BUFFER_SIZE];
static RingBuffer_t usart_rx_buffer;

/* IT transfer state, written from SPI_ApplicationEventCallback() (interrupt context)
 * and read from the main loop -- volatile for the same reason as the equivalent state
 * in App 031. */
static volatile uint8_t spi_tx_buffer[MAX_SPI_TX_BYTES];
static volatile uint8_t spi_last_len;
static volatile uint8_t spi_transfer_done;
static volatile uint8_t spi_last_event;

static void USART1_GPIOInits(void);
static void USART1_Inits(void);
static void USART1_RXInterruptEnable(void);
static void SPI1_GPIOInits(void);
static void SPI1_Inits(void);
static void SPI1_IRQInterruptEnable(void);
static void SPI_CS_Low(void);
static void SPI_CS_High(void);

static uint8_t ParseHexByte(const char **text, uint8_t *out);
static uint8_t ParseHexList(const char *text, volatile uint8_t *out, uint8_t max_count, uint8_t *out_count);

static void SendString(const char *text);
static void SendHex8(uint8_t value);

static void ProcessCommand(uint8_t *cmd);

static void USART1_GPIOInits(void)
{
	GPIO_Handle_t USARTPins;

	USARTPins.pGPIOx = GPIOB;
	USARTPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	USARTPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	USARTPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	USARTPins.GPIO_PinConfig.GPIO_PinAltFunMode = 7;
	USARTPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&USARTPins);

	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&USARTPins);
}

static void USART1_Inits(void)
{
	USART1Handle.pUSARTx = USART1;
	USART1Handle.USART_Config.USART_Baud = USART_STD_BAUD_9600;
	USART1Handle.USART_Config.USART_Mode = USART_MODE_TXRX;
	USART1Handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
	USART1Handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
	USART1Handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;
	USART1Handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;

	USART_Init(&USART1Handle);
}

static void USART1_RXInterruptEnable(void)
{
	USART_EnableRXNEInterrupt(&USART1Handle);
	USART_EnableErrorInterrupts(&USART1Handle);

	USART_IRQPriorityConfig(IRQ_NO_USART1, NVIC_IRQ_PRI15);
	USART_IRQInterruptConfig(IRQ_NO_USART1, ENABLE);
}

static void SPI1_GPIOInits(void)
{
	GPIO_Handle_t SPIPins;
	GPIO_Handle_t CSPin;

	SPIPins.pGPIOx = GPIOA;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;   /* AF5 = SPI1 on PA5/PA6/PA7 */
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GPIO_Init(&SPIPins);

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&SPIPins);

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&SPIPins);

	/* CS is a plain GPIO output, not AF -- driven manually, same pattern as the SD-over-SPI
	 * driver's chip-select (App 030). */
	CSPin.pGPIOx = SPI_CS_PORT;
	CSPin.GPIO_PinConfig.GPIO_PinNumber = SPI_CS_PIN;
	CSPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	CSPin.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	CSPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	CSPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;

	GPIO_Init(&CSPin);
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, 1U);   /* deselected by default */
}

static void SPI1_Inits(void)
{
	SPI1Handle.pSPIx = SPI1;
	SPI1Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI1Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI1Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI1Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI1Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;        /* mode 0 -- what the RC522 expects */
	SPI1Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV32;   /* slow on purpose, easy to read on a logic analyzer */
	SPI1Handle.SPIConfig.SPI_SSM = SPI_SSM_EN;

	SPI_Init(&SPI1Handle);
	SPI_SSIConfig(SPI1, ENABLE);       /* software NSS management, keep internal NSS high */
	SPI_PeripheralControl(SPI1, ENABLE);
}

static void SPI1_IRQInterruptEnable(void)
{
	SPI_IRQPriorityConfig(IRQ_NO_SPI1, NVIC_IRQ_PRI15);
	SPI_IRQInterruptConfig(IRQ_NO_SPI1, ENABLE);
}

static void SPI_CS_Low(void)
{
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, 0U);
}

static void SPI_CS_High(void)
{
	GPIO_WriteToOutputPin(SPI_CS_PORT, SPI_CS_PIN, 1U);
}

/* Parses one or two hex digits starting at *text, advancing *text past what it
 * consumed. Returns 0 (and leaves *text and *out untouched) if there is no valid
 * hex digit at the current position. */
static uint8_t ParseHexByte(const char **text, uint8_t *out)
{
	uint8_t value = 0;
	uint8_t digits = 0;
	const char *p = *text;

	while (digits < 2U)
	{
		char c = *p;
		uint8_t nibble;

		if ((c >= '0') && (c <= '9'))
		{
			nibble = (uint8_t)(c - '0');
		}
		else if ((c >= 'A') && (c <= 'F'))
		{
			nibble = (uint8_t)(c - 'A' + 10);
		}
		else if ((c >= 'a') && (c <= 'f'))
		{
			nibble = (uint8_t)(c - 'a' + 10);
		}
		else
		{
			break;
		}

		value = (uint8_t)((value << 4) | nibble);
		p++;
		digits++;
	}

	if (digits == 0U)
	{
		return 0;
	}

	*text = p;
	*out = value;
	return 1;
}

/* Parses a space-separated list of hex bytes (up to max_count) into out. Returns 0
 * if there wasn't at least one valid byte. */
static uint8_t ParseHexList(const char *text, volatile uint8_t *out, uint8_t max_count, uint8_t *out_count)
{
	uint8_t count = 0;

	while (count < max_count)
	{
		uint8_t value = 0;

		while (*text == ' ')
		{
			text++;
		}

		if (*text == '\0')
		{
			break;
		}

		if (!ParseHexByte(&text, &value))
		{
			return 0;
		}

		out[count] = value;
		count++;
	}

	if (count == 0U)
	{
		return 0;
	}

	*out_count = count;
	return 1;
}

int main(void)
{
	uint8_t rx;
	uint8_t cmd_buffer[CMD_BUFFER_SIZE];
	uint32_t cmd_index = 0;

	SystemClock_HSI_Init();
	HSI16_ENABLE();

	USART1_CCLK_HSI16();

	USART1_GPIOInits();
	SPI1_GPIOInits();

	USART1_Inits();
	SPI1_Inits();
	RingBuffer_Init(&usart_rx_buffer, rx_storage, RX_BUFFER_SIZE);
	USART1_RXInterruptEnable();
	SPI1_IRQInterruptEnable();

	SendString("READY SPI_IT\r\n");

	while (1)
	{
		if (RingBuffer_Read(&usart_rx_buffer, &rx))
		{
			if ((rx == '\r') || (rx == '\n'))
			{
				if (cmd_index > 0U)
				{
					cmd_buffer[cmd_index] = '\0';
					ProcessCommand(cmd_buffer);
					cmd_index = 0;
				}
			}
			else
			{
				if (cmd_index < (CMD_BUFFER_SIZE - 1U))
				{
					cmd_buffer[cmd_index++] = rx;
				}
				else
				{
					cmd_index = 0;
					SendString("ERR:CMD_TOO_LONG\r\n");
				}
			}
		}
	}
}

static void ProcessCommand(uint8_t *cmd)
{
	if (strcmp((char *)cmd, "PING") == 0)
	{
		SendString("PONG\r\n");
	}
	else if (strcmp((char *)cmd, "HELP") == 0)
	{
		SendString("CMDS:PING,APP_ID?,SPI RESET,SPI SEND <hex bytes>,SPI STATUS\r\n");
	}
	else if (strcmp((char *)cmd, "APP_ID?") == 0)
	{
		SendString("SPI_INTERRUPT_MASTERTX\r\n");
	}
	else if (strcmp((char *)cmd, "SPI RESET") == 0)
	{
		if (SPI1Handle.TxState != SPI_READY)
		{
			SendString("ERR:SPI_BUSY\r\n");
		}
		else
		{
			uint8_t status;

			/* MFRC522: write CommandReg (0x01) = SoftReset (0x0F). Address byte for a
			 * write is (reg << 1) & 0x7E -- (0x01 << 1) & 0x7E = 0x02. */
			spi_tx_buffer[0] = 0x02U;
			spi_tx_buffer[1] = 0x0FU;
			spi_last_len = 2U;
			spi_transfer_done = 0U;

			SPI_CS_Low();
			status = SPI_SendDataIT(&SPI1Handle, (uint8_t *)spi_tx_buffer, 2U);

			if (status != SPI_OK)
			{
				SPI_CS_High();
				SendString("ERR:SPI_START_FAILED\r\n");
			}
			else
			{
				SendString("SPI_RESET STARTED\r\n");
			}
		}
	}
	else if (strncmp((char *)cmd, "SPI SEND ", 9) == 0)
	{
		if (SPI1Handle.TxState != SPI_READY)
		{
			SendString("ERR:SPI_BUSY\r\n");
		}
		else
		{
			uint8_t count = 0;

			if (ParseHexList((char *)cmd + 9, spi_tx_buffer, MAX_SPI_TX_BYTES, &count))
			{
				uint8_t status;

				spi_last_len = count;
				spi_transfer_done = 0U;

				SPI_CS_Low();
				status = SPI_SendDataIT(&SPI1Handle, (uint8_t *)spi_tx_buffer, count);

				if (status != SPI_OK)
				{
					SPI_CS_High();
					SendString("ERR:SPI_START_FAILED\r\n");
				}
				else
				{
					SendString("SPI_SEND STARTED LEN=");
					SendHex8(count);
					SendString("\r\n");
				}
			}
			else
			{
				SendString("ERR:BAD_ARG\r\n");
			}
		}
	}
	else if (strcmp((char *)cmd, "SPI STATUS") == 0)
	{
		SendString("STATE=");
		SendString((SPI1Handle.TxState == SPI_READY) ? "READY" : "BUSY_TX");
		SendString(" LAST_EVENT=");
		SendHex8(spi_last_event);
		SendString(" LAST_LEN=");
		SendHex8(spi_last_len);
		SendString(" DONE=");
		SendString(spi_transfer_done ? "1" : "0");
		SendString("\r\n");
	}
	else
	{
		SendString("ERR\r\n");
	}
}

static void SendString(const char *text)
{
	USART_SendData(&USART1Handle, (uint8_t *)text, strlen(text));
}

static void SendHex8(uint8_t value)
{
	static const char hex_digits[] = "0123456789ABCDEF";
	uint8_t buffer[4];

	buffer[0] = '0';
	buffer[1] = 'x';
	buffer[2] = (uint8_t)hex_digits[(value >> 4) & 0x0FU];
	buffer[3] = (uint8_t)hex_digits[value & 0x0FU];

	USART_SendData(&USART1Handle, buffer, sizeof(buffer));
}

void USART1_IRQHandler(void)
{
	USART_IRQHandling(&USART1Handle);
}

void SPI1_IRQHandler(void)
{
	SPI_IRQHandling(&SPI1Handle);
}

void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t AppEv)
{
	if (AppEv == USART_EVENT_RXNE)
	{
		RingBuffer_Write(&usart_rx_buffer, pUSARTHandle->RxByte);
	}
}

/*
 * This is the payoff: no while loop anywhere in this file waits on a SPI flag. CS
 * goes low, SPI_SendDataIT() kicks the transfer off and returns immediately, and
 * every byte after that -- plus raising CS back up once the frame is done -- happens
 * here, from the ISR.
 */
void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{
	spi_last_event = AppEv;

	if (AppEv == SPI_EVENT_TX_CMPLT)
	{
		SPI_CS_High();
		spi_transfer_done = 1U;
	}
	else if (AppEv == SPI_EVENT_OVR_ERR)
	{
		SPI_CS_High();
		spi_transfer_done = 1U;
		SPI_ClearOVRFlag(pSPIHandle->pSPIx);
	}
}
