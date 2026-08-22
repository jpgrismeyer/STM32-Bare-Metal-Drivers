/*
 * sd_spi_driver.c
 *
 * SD-over-SPI protocol implementation: initialization (SD_Init) plus
 * single-block read/write (SD_ReadBlock/SD_WriteBlock). See sd_spi_driver.h
 * for the exact scope of this version (raw block I/O only -- no FatFs glue
 * or multi-block transfers yet, that's the next milestone).
 *
 * Reference protocol (standard, not vendor-specific): SD Physical Layer
 * Simplified Specification, "SPI Mode" chapter. Every command/response and
 * data-token layout used below (CMD0, CMD8, CMD55/ACMD41, CMD58, CMD16,
 * CMD17, CMD24, R1/R3/R7 responses, start/data-response tokens) comes from
 * that public specification.
 */

#include "sd_spi_driver.h"
#include <stddef.h>

static void    SD_CS_Low(const SD_Handle_t *pSDHandle);
static void    SD_CS_High(const SD_Handle_t *pSDHandle);
static uint8_t SD_SPI_TransferByte(SPI_RegDef_t *pSPIx, uint8_t tx_byte);
static void    SD_SPI_SendDummyBytes(SPI_RegDef_t *pSPIx, uint32_t count);
static uint8_t SD_SendCommand(SD_Handle_t *pSDHandle, uint8_t cmd, uint32_t arg,
                               uint8_t crc, uint8_t *response, uint8_t response_len);
static void    SD_SPI_SetFastClock(SPI_RegDef_t *pSPIx);

static void SD_CS_Low(const SD_Handle_t *pSDHandle)
{
	GPIO_WriteToOutputPin(pSDHandle->pCSPort, pSDHandle->CSPin, 0U);
}

static void SD_CS_High(const SD_Handle_t *pSDHandle)
{
	GPIO_WriteToOutputPin(pSDHandle->pCSPort, pSDHandle->CSPin, 1U);
}

/*
 * Full-duplex single-byte exchange. The core SPI driver's SendDataWithStatus
 * / ReceiveDataWithStatus are one-directional helpers (built for the
 * transmit-only and receive-only examples in App 006-008) -- the SD-over-SPI
 * protocol needs to clock a byte out and read the simultaneously shifted-in
 * byte back on every single transfer (that's how command responses and dummy
 * clocking both work), so this talks to the SPI registers directly, same
 * register-level pattern SPI_SendDataWithStatus itself uses internally.
 */
static uint8_t SD_SPI_TransferByte(SPI_RegDef_t *pSPIx, uint8_t tx_byte)
{
	uint32_t timeout;

	timeout = SPI_TIMEOUT_COUNT;
	while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == FLAG_RESET)
	{
		if (timeout-- == 0U)
		{
			return 0xFFU;
		}
	}
	*((__vo uint8_t *)&pSPIx->DR) = tx_byte;

	timeout = SPI_TIMEOUT_COUNT;
	while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == FLAG_RESET)
	{
		if (timeout-- == 0U)
		{
			return 0xFFU;
		}
	}
	return *((__vo uint8_t *)&pSPIx->DR);
}

static void SD_SPI_SendDummyBytes(SPI_RegDef_t *pSPIx, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++)
	{
		(void)SD_SPI_TransferByte(pSPIx, 0xFFU);
	}
}

/*
 * Sends a 6-byte SD command frame (start bit + command index, 4-byte
 * argument big-endian, CRC + stop bit), then polls for the R1 response
 * (first byte clocked back with bit 7 clear). If response_len > 1, reads
 * the extra trailing bytes some commands return (R3/R7 formats) right
 * after R1.
 *
 * 'crc' must be a real, valid CRC7+stop-bit byte for CMD0 (0x95) and CMD8
 * (0x87) -- those two are the only commands where the SD spec requires CRC
 * checking even in SPI mode, because the card hasn't necessarily switched
 * out of CRC-checking mode yet. Every other command in this driver passes
 * 0x01 (stop bit only), which is safe because CRC checking is off by
 * default in SPI mode once CMD0 has succeeded.
 */
static uint8_t SD_SendCommand(SD_Handle_t *pSDHandle, uint8_t cmd, uint32_t arg,
                               uint8_t crc, uint8_t *response, uint8_t response_len)
{
	uint8_t frame[6];
	uint8_t r1 = 0xFFU;
	uint8_t i;

	frame[0] = 0x40U | cmd;
	frame[1] = (uint8_t)(arg >> 24);
	frame[2] = (uint8_t)(arg >> 16);
	frame[3] = (uint8_t)(arg >> 8);
	frame[4] = (uint8_t)arg;
	frame[5] = crc;

	for (i = 0; i < 6U; i++)
	{
		(void)SD_SPI_TransferByte(pSDHandle->pSPIx, frame[i]);
	}

	for (i = 0; i < 10U; i++)
	{
		r1 = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
		if ((r1 & 0x80U) == 0U)
		{
			break;
		}
	}

	if (response != NULL)
	{
		response[0] = r1;
		for (i = 1U; i < response_len; i++)
		{
			response[i] = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
		}
	}

	return r1;
}

/*
 * Initialization must run at <=400kHz-equivalent (caller's responsibility,
 * via the SPI_Init() prescaler passed in before SD_Init() is called). Real
 * block read/write is far too slow at that speed, so once the card is
 * ready this switches the SPI peripheral to a faster prescaler. SPE must be
 * cleared while changing BR per the reference manual, so this can't just be
 * a register OR.
 */
static void SD_SPI_SetFastClock(SPI_RegDef_t *pSPIx)
{
	pSPIx->CR1 &= ~(1U << SPI_CR1_SPE);
	pSPIx->CR1 &= ~(0x7U << SPI_CR1_BR);
	pSPIx->CR1 |= ((uint32_t)SPI_SCLK_SPEED_DIV4 << SPI_CR1_BR);
	pSPIx->CR1 |= (1U << SPI_CR1_SPE);
}

uint8_t SD_Init(SD_Handle_t *pSDHandle)
{
	uint8_t response[5];
	uint8_t r1;
	uint32_t retry;

	if ((pSDHandle == NULL) || (pSDHandle->pSPIx == NULL) || (pSDHandle->pCSPort == NULL))
	{
		return SD_ERROR;
	}

	pSDHandle->CardType = SD_CARD_TYPE_UNKNOWN;

	/* Power-up: >=74 clock cycles with CS held high before the card sees any command. */
	SD_CS_High(pSDHandle);
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 10U);

	/* CMD0: GO_IDLE_STATE -- switches the card into SPI mode. Needs a real CRC (0x95). */
	SD_CS_Low(pSDHandle);
	r1 = SD_SendCommand(pSDHandle, 0U, 0x00000000UL, 0x95U, response, 1U);
	if (r1 != 0x01U)
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR_NO_CARD;
	}

	/* CMD8: SEND_IF_COND -- voltage range 2.7-3.6V (0x1), check pattern 0xAA. Needs CRC 0x87. */
	r1 = SD_SendCommand(pSDHandle, 8U, 0x000001AAUL, 0x87U, response, 5U);
	if ((r1 & 0x04U) != 0U)
	{
		/* Illegal command: not a ver2.0+ SD card (ver1.x SD or MMC). Out of scope for now. */
		SD_CS_High(pSDHandle);
		return SD_ERROR_UNSUPPORTED;
	}
	if ((r1 != 0x01U) || (response[3] != 0x01U) || (response[4] != 0xAAU))
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR_VOLTAGE;
	}

	/* CMD55+ACMD41 until the card leaves idle state. Arg bit 30 = HCS (we support SDHC/SDXC). */
	retry = 20000U;
	do
	{
		(void)SD_SendCommand(pSDHandle, 55U, 0x00000000UL, 0x01U, response, 1U);
		r1 = SD_SendCommand(pSDHandle, 41U, 0x40000000UL, 0x01U, response, 1U);
		retry--;
	} while ((r1 != 0x00U) && (retry > 0U));

	if (r1 != 0x00U)
	{
		SD_CS_High(pSDHandle);
		return SD_TIMEOUT;
	}

	/* CMD58: READ_OCR. Bit 30 of the OCR (bit 6 of response[1], the OCR's MSB) is CCS:
	 * 1 = SDHC/SDXC (native 512B block addressing), 0 = SDSC (byte addressing). */
	r1 = SD_SendCommand(pSDHandle, 58U, 0x00000000UL, 0x01U, response, 5U);
	if (r1 != 0x00U)
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR;
	}

	if ((response[1] & 0x40U) != 0U)
	{
		pSDHandle->CardType = SD_CARD_TYPE_SDHC_SDXC;
	}
	else
	{
		pSDHandle->CardType = SD_CARD_TYPE_SDSC;

		/* SDSC cards can default to a non-512-byte block length; force it so every
		 * later block read/write can assume the same size as SDHC/SDXC. */
		r1 = SD_SendCommand(pSDHandle, 16U, (uint32_t)SD_BLOCK_SIZE, 0x01U, response, 1U);
		if (r1 != 0x00U)
		{
			SD_CS_High(pSDHandle);
			return SD_ERROR;
		}
	}

	SD_CS_High(pSDHandle);
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 1U);

	SD_SPI_SetFastClock(pSDHandle->pSPIx);

	return SD_OK;
}

/* SDSC needs a byte address (block_addr * 512); SDHC/SDXC address blocks directly. */
static uint32_t SD_BlockToAddress(const SD_Handle_t *pSDHandle, uint32_t block_addr)
{
	if (pSDHandle->CardType == SD_CARD_TYPE_SDHC_SDXC)
	{
		return block_addr;
	}

	return block_addr * SD_BLOCK_SIZE;
}

#define SD_DATA_TOKEN_START_BLOCK   0xFEU
#define SD_TOKEN_WAIT_RETRIES       50000U
#define SD_BUSY_WAIT_RETRIES        200000U

uint8_t SD_ReadBlock(SD_Handle_t *pSDHandle, uint32_t block_addr, uint8_t *buffer)
{
	uint8_t response[1];
	uint8_t r1;
	uint8_t token = 0xFFU;
	uint32_t retry;
	uint32_t i;

	if ((pSDHandle == NULL) || (buffer == NULL))
	{
		return SD_ERROR;
	}

	SD_CS_Low(pSDHandle);

	r1 = SD_SendCommand(pSDHandle, 17U, SD_BlockToAddress(pSDHandle, block_addr), 0x01U, response, 1U);
	if (r1 != 0x00U)
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR;
	}

	/* Wait for the data start token. The card can take a while to have the block ready. */
	retry = SD_TOKEN_WAIT_RETRIES;
	do
	{
		token = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
		retry--;
	} while ((token == 0xFFU) && (retry > 0U));

	if (token != SD_DATA_TOKEN_START_BLOCK)
	{
		SD_CS_High(pSDHandle);
		return SD_TIMEOUT;
	}

	for (i = 0; i < SD_BLOCK_SIZE; i++)
	{
		buffer[i] = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
	}

	/* 2 trailing CRC bytes, not checked (CRC checking is off by default in SPI mode). */
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 2U);

	SD_CS_High(pSDHandle);
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 1U);

	return SD_OK;
}

uint8_t SD_WriteBlock(SD_Handle_t *pSDHandle, uint32_t block_addr, const uint8_t *buffer)
{
	uint8_t response[1];
	uint8_t r1;
	uint8_t data_response;
	uint32_t retry;
	uint32_t i;

	if ((pSDHandle == NULL) || (buffer == NULL))
	{
		return SD_ERROR;
	}

	SD_CS_Low(pSDHandle);

	r1 = SD_SendCommand(pSDHandle, 24U, SD_BlockToAddress(pSDHandle, block_addr), 0x01U, response, 1U);
	if (r1 != 0x00U)
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR;
	}

	(void)SD_SPI_TransferByte(pSDHandle->pSPIx, SD_DATA_TOKEN_START_BLOCK);

	for (i = 0; i < SD_BLOCK_SIZE; i++)
	{
		(void)SD_SPI_TransferByte(pSDHandle->pSPIx, buffer[i]);
	}

	/* 2 dummy CRC bytes -- CRC checking is off by default in SPI mode, value doesn't matter. */
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 2U);

	/* Data response token: low nibble pattern 0bxxx00101 = accepted. Any other pattern
	 * (CRC error 0bxxx01011, write error 0bxxx01101) means the card rejected the block. */
	data_response = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
	if ((data_response & 0x1FU) != 0x05U)
	{
		SD_CS_High(pSDHandle);
		return SD_ERROR_WRITE;
	}

	/* Card holds MISO low (reads back 0x00) while programming the block internally. */
	retry = SD_BUSY_WAIT_RETRIES;
	do
	{
		r1 = SD_SPI_TransferByte(pSDHandle->pSPIx, 0xFFU);
		retry--;
	} while ((r1 == 0x00U) && (retry > 0U));

	SD_CS_High(pSDHandle);
	SD_SPI_SendDummyBytes(pSDHandle->pSPIx, 1U);

	if (retry == 0U)
	{
		return SD_TIMEOUT;
	}

	return SD_OK;
}
