/*
 * sd_spi_driver.h
 *
 * SD card block-device driver running the SD-over-SPI protocol on top of the
 * existing bare-metal SPI driver (Inc/Src/stm32l475xx_spi_driver.*). This is
 * NOT a new MCU peripheral driver -- it is a device driver, same category as
 * how App/030 talks to the HTS221/LPS22HB sensors over I2C, except this one
 * is reused across future milestones so it lives in Inc/Src instead of
 * being embedded as static functions in one App .c file.
 *
 * Scope of this version (see Plan_Contenido_STM32.md, section 3):
 *   - SD_Init(): full SPI-mode init sequence (CMD0, CMD8, ACMD41, CMD58,
 *     optional CMD16), detects SDSC vs SDHC/SDXC addressing.
 *   - SD_ReadBlock()/SD_WriteBlock(): single 512-byte block read (CMD17)
 *     and write (CMD24) using the standard SPI-mode data token framing.
 *   - Not yet implemented: anything above raw block I/O (FatFs glue,
 *     multi-block transfers) -- that's the next milestone.
 *
 * Only SD ver2.00+ cards (SDSC/SDHC/SDXC) are supported. Very old ver1.x
 * cards or MMC cards are out of scope -- practically all microSD cards sold
 * today are ver2.0+, so this matches the "buy a modern microSD" shopping
 * list entry in the plan doc.
 */

#ifndef INC_SD_SPI_DRIVER_H_
#define INC_SD_SPI_DRIVER_H_

#include "stm32l47xx.h"
#include "stm32l475xx_spi_driver.h"
#include "stm32l475xx_gpio_driver.h"

/* Card type, filled in by SD_Init() once the card responds. */
#define SD_CARD_TYPE_UNKNOWN     0U
#define SD_CARD_TYPE_SDSC        1U   /* Standard Capacity -- byte addressing, needs CMD16 */
#define SD_CARD_TYPE_SDHC_SDXC   2U   /* High/Extended Capacity -- native 512B block addressing */

/* Return codes, used consistently by every function in this driver. */
#define SD_OK                    0U
#define SD_ERROR                 1U
#define SD_TIMEOUT               2U
#define SD_ERROR_NO_CARD         3U   /* card never left idle state / never responded */
#define SD_ERROR_UNSUPPORTED     4U   /* card answered but is not a ver2.0+ SD card (CMD8 rejected) */
#define SD_ERROR_VOLTAGE         5U   /* CMD8 echoed a different check pattern / voltage range */
#define SD_ERROR_WRITE           6U   /* card rejected the data block (CRC or write error token) */

#define SD_BLOCK_SIZE            512U

/*
 * SD_Handle_t bundles the SPI peripheral used for the card plus the GPIO
 * pin used as chip-select. Chip-select is driven manually (plain GPIO
 * output, not hardware NSS) because the SD-over-SPI protocol needs CS held
 * in specific states around dummy clock pulses that hardware NSS control
 * doesn't give us.
 *
 * The caller is responsible for SPI_Init() on pSPIx before calling
 * SD_Init(): CPOL=0, CPHA=0 (SPI mode 0), 8-bit frames, master mode, slow
 * clock (<=400kHz-equivalent prescaler, e.g. SPI_SCLK_SPEED_DIV256 off a
 * 16MHz kernel clock) for the initialization sequence. SD_Init() switches
 * the peripheral to a faster prescaler once the card is ready.
 */
typedef struct
{
	SPI_RegDef_t   *pSPIx;
	GPIO_RegDef_t  *pCSPort;
	uint8_t         CSPin;
	uint8_t         CardType;
} SD_Handle_t;

/*
 * Runs the full SD-over-SPI initialization sequence:
 *   1. >=74 dummy clocks with CS high (card power-up requirement).
 *   2. CMD0 (GO_IDLE_STATE) -> card must answer R1=0x01 (idle).
 *   3. CMD8 (SEND_IF_COND, voltage 2.7-3.6V, check pattern 0xAA) -> confirms
 *      a ver2.0+ card and that it accepts our voltage range.
 *   4. CMD55+ACMD41 (host-capacity-support SD_SEND_OP_COND), retried until
 *      the card leaves idle state (R1=0x00) or a retry budget is exhausted.
 *   5. CMD58 (READ_OCR) -> reads the CCS bit to tell SDSC apart from
 *      SDHC/SDXC (this decides whether block addresses need to be
 *      multiplied by 512 for later read/write commands).
 *   6. CMD16 (SET_BLOCKLEN=512), only sent for SDSC cards.
 *   7. Switches the SPI peripheral to a faster clock prescaler for normal
 *      operation.
 *
 * Returns SD_OK on success; pSDHandle->CardType is valid only after SD_OK.
 */
uint8_t SD_Init(SD_Handle_t *pSDHandle);

/*
 * Reads/writes one 512-byte block. 'block_addr' is a logical block number
 * (0, 1, 2, ...), NOT a byte offset -- for SDSC cards this driver converts
 * it to a byte address internally (block_addr * 512); SDHC/SDXC cards use
 * block addressing natively. 'buffer' must be exactly SD_BLOCK_SIZE bytes.
 *
 * Both require SD_Init() to have returned SD_OK first (uses pSDHandle's
 * CardType to pick the right addressing mode).
 */
uint8_t SD_ReadBlock(SD_Handle_t *pSDHandle, uint32_t block_addr, uint8_t *buffer);
uint8_t SD_WriteBlock(SD_Handle_t *pSDHandle, uint32_t block_addr, const uint8_t *buffer);

#endif /* INC_SD_SPI_DRIVER_H_ */
