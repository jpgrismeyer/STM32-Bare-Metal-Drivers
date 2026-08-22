#include "stm32l47xx.h"
#include "ring_buffer.h"
#include "sd_spi_driver.h"
#include <string.h>

#define MY_ADDR                 0x61

#define LPS22HB_ADDR            0x5D
#define LPS22HB_WHO_AM_I_REG    0x0F
#define LPS22HB_WHO_AM_I_VAL    0xB1
#define LPS22HB_CTRL_REG1       0x10
#define LPS22HB_PRESS_OUT_XL    0x28
#define LPS22HB_PRESS_OUT_L     0x29
#define LPS22HB_PRESS_OUT_H     0x2A

#define HTS221_ADDR             0x5F
#define HTS221_WHO_AM_I_REG     0x0F
#define HTS221_WHO_AM_I_VAL     0xBC
#define HTS221_CTRL_REG1        0x20
#define HTS221_HUMIDITY_OUT_L   0x28
#define HTS221_HUMIDITY_OUT_H   0x29
#define HTS221_TEMP_OUT_L       0x2A
#define HTS221_TEMP_OUT_H       0x2B

/* HTS221 factory calibration registers (see ST datasheet, section calibration) */
#define HTS221_H0_RH_X2         0x30
#define HTS221_H1_RH_X2         0x31
#define HTS221_T0_DEGC_X8       0x32
#define HTS221_T1_DEGC_X8       0x33
#define HTS221_T1_T0_MSB        0x35
#define HTS221_H0_OUT_L         0x36
#define HTS221_H0_OUT_H         0x37
#define HTS221_H1_OUT_L         0x3A
#define HTS221_H1_OUT_H         0x3B
#define HTS221_T0_OUT_L         0x3C
#define HTS221_T0_OUT_H         0x3D
#define HTS221_T1_OUT_L         0x3E
#define HTS221_T1_OUT_H         0x3F

/*
 * Fan actuator (Plant Guardian): drives a relay that switches a 5V fan when
 * air humidity drops below a configurable threshold. Any free GPIO works --
 * PA8 is unused by this app (USART1 is on PB6/PB7, I2C2 is on PB10/PB11, I2C1
 * is on PB8/PB9, the onboard LED is PB14). Confirm against your specific
 * board wiring before connecting the relay.
 */
#define FAN_GPIO_PORT            GPIOA
#define FAN_GPIO_PIN             GPIO_PIN_NO_8
#define HUMIDITY_THRESHOLD_DEFAULT_X10  400   /* 40.0 %RH default, tune by testing */

/*
 * SD card over SPI1 (Plant Guardian data logger). PA5/PA6/PA7 are SPI1
 * SCK/MISO/MOSI (AF5, same peripheral/AF used in App 006-008), PA4 is a
 * plain GPIO output used as chip-select (software-controlled, not hardware
 * NSS -- the SD-over-SPI protocol needs specific CS timing around dummy
 * clock pulses that hardware NSS control doesn't give us).
 */
#define SD_SPI_PORT              GPIOA
#define SD_SCK_PIN               GPIO_PIN_NO_5
#define SD_MISO_PIN              GPIO_PIN_NO_6
#define SD_MOSI_PIN              GPIO_PIN_NO_7
#define SD_CS_PIN                GPIO_PIN_NO_4

/*
 * Block used by SD WRITE TEST / SD READ TEST. Deliberately far from block 0
 * (where the MBR/partition table lives) so the round-trip test doesn't
 * touch anything a partition or filesystem might already occupy. Still:
 * only run these commands on a card you don't mind overwriting raw data
 * on -- there is no filesystem layer yet (that's the FatFs milestone),
 * so this writes straight to the raw block.
 */
#define SD_TEST_BLOCK_ADDR       2000UL

#define CMD_BUFFER_SIZE         64
#define RX_BUFFER_SIZE          128

USART_Handle_t USART1Handle;
I2C_Handle_t I2C2Handle;
/*
 * I2C1 (PB8=SCL, PB9=SDA) is only used for the "external module" commands
 * below (I2C1 SCAN / I2C1 READX). Unlike I2C2 (wired internally to the
 * onboard sensors, not exposed on any header -- confirmed against UM2153),
 * I2C1 IS broken out on the ARDUINO Uno V3 connector (CN1, pins D15/D14),
 * so it's the bus to use for anything you wire up externally and want to
 * probe with a logic analyzer.
 */
I2C_Handle_t I2C1Handle;
SPI_Handle_t SPI1Handle;
static SD_Handle_t sdHandle;
static uint8_t sd_init_done;
static uint8_t sd_test_buffer[SD_BLOCK_SIZE];

/* HTS221 factory calibration, read once in Sensors_Init() and reused for every conversion. */
typedef struct
{
	float   T0_degC;
	float   T1_degC;
	int16_t T0_OUT;
	int16_t T1_OUT;
	float   H0_rH;
	float   H1_rH;
	int16_t H0_OUT;
	int16_t H1_OUT;
} HTS221_Calib_t;

static HTS221_Calib_t hts221_calib;
static uint8_t hts221_calib_valid;

static int32_t humidity_threshold_x10 = HUMIDITY_THRESHOLD_DEFAULT_X10;
static uint8_t fan_state;

static volatile uint8_t rx_storage[RX_BUFFER_SIZE];
static RingBuffer_t usart_rx_buffer;
static volatile uint32_t usart_error_events;

static uint32_t IRQ_SaveAndDisable(void);
static void IRQ_Restore(uint32_t primask);

static void USART1_GPIOInits(void);
static void USART1_Inits(void);
static void USART1_RXInterruptEnable(void);
static void I2C2_GPIOInits(void);
static void I2C2_Inits(void);
static void I2C1_GPIOInits(void);
static void I2C1_Inits(void);
static void GPIO_LEDInit(void);
static void GPIO_FanInit(void);
static void SD_SPI1_GPIOInits(void);
static void SD_SPI1_Inits(void);
static void Fan_UpdateState(int32_t hum_rh_x10);
static uint8_t ParseInt32(const char *text, int32_t *out);

static void SendString(const char *text);
static void SendHex8(uint8_t value);
static void SendInt32(int32_t value);
static void SendStatus(uint8_t status);
static void SendUSARTStatus(void);
static void SendSDStatus(uint8_t status);

static uint8_t I2C_ReadReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t *value);
static uint8_t I2C_WriteReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t value);
static uint8_t I2C1_ReadReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t *value);
static uint8_t ParseHexByte(const char **text, uint8_t *out);
static uint8_t ParseHex8Pair(const char *text, uint8_t *a, uint8_t *b);
static uint8_t Sensor_ReadId(uint8_t slave_addr, uint8_t *id);
static uint8_t Sensors_Init(void);
static uint8_t LPS22HB_ReadPressureRaw(int32_t *pressure_raw);
static uint8_t HTS221_ReadTemperatureRaw(int16_t *temperature_raw);
static uint8_t HTS221_ReadHumidityRaw(int16_t *humidity_raw);
static uint8_t HTS221_ReadCalibration(HTS221_Calib_t *calib);
static int32_t LPS22HB_ConvertPressureHPa_x10(int32_t pressure_raw);
static int32_t HTS221_ConvertTemperatureC_x10(int16_t temperature_raw, const HTS221_Calib_t *calib);
static int32_t HTS221_ConvertHumidityRH_x10(int16_t humidity_raw, const HTS221_Calib_t *calib);
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

static void I2C2_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
	GPIO_Init(&I2CPins);

	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
	GPIO_Init(&I2CPins);
}

static void I2C2_Inits(void)
{
	I2C2Handle.pI2Cx = I2C2;
	I2C2Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C2Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C2Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;
	I2C2Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_DISABLE;

	I2C_Init(&I2C2Handle);
}

/*
 * I2C1 on PB8 (SCL) / PB9 (SDA), AF4 open-drain with pull-ups -- same pattern
 * as I2C2_GPIOInits(), different pins. This is the bus exposed on the
 * ARDUINO Uno V3 header (CN1, D15/D14), meant for whatever external module
 * gets wired up for logic-analyzer captures.
 */
static void I2C1_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	I2CPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_8;
	GPIO_Init(&I2CPins);

	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_9;
	GPIO_Init(&I2CPins);
}

static void I2C1_Inits(void)
{
	I2C1Handle.pI2Cx = I2C1;
	I2C1Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C1Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C1Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;
	I2C1Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_DISABLE;

	I2C_Init(&I2C1Handle);
}

static void GPIO_LEDInit(void)
{
	GPIO_Handle_t GPIOLED;

	GPIOLED.pGPIOx = GPIOB;
	GPIOLED.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	GPIOLED.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	GPIOLED.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	GPIOLED.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOLED.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOLED);
}

static void GPIO_FanInit(void)
{
	GPIO_Handle_t GPIOFan;

	GPIOFan.pGPIOx = FAN_GPIO_PORT;
	GPIOFan.GPIO_PinConfig.GPIO_PinNumber = FAN_GPIO_PIN;
	GPIOFan.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	GPIOFan.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	GPIOFan.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOFan.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOFan);
	GPIO_WriteToOutputPin(FAN_GPIO_PORT, FAN_GPIO_PIN, 0U);   /* fan OFF by default */
}

/*
 * Turns the fan on when humidity drops below the configured threshold, off
 * once it's back at or above it. Called right after every fresh humidity
 * reading (HUM RH, MONITOR FULL), so the fan reacts on every poll from the
 * host.
 *
 * NOTE: this app has no periodic timer tick, so the fan only updates when a
 * humidity-reading command actually runs. For unattended operation (nobody
 * polling over USART), the natural next step is a SysTick-driven loop that
 * reads sensors and updates the fan on its own -- not implemented yet.
 */
static void Fan_UpdateState(int32_t hum_rh_x10)
{
	fan_state = (hum_rh_x10 < humidity_threshold_x10) ? 1U : 0U;
	GPIO_WriteToOutputPin(FAN_GPIO_PORT, FAN_GPIO_PIN, fan_state);
}

static void SD_SPI1_GPIOInits(void)
{
	GPIO_Handle_t SPIPins;
	GPIO_Handle_t CSPin;

	SPIPins.pGPIOx = SD_SPI_PORT;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	SPIPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	SPIPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;
	SPIPins.GPIO_PinConfig.GPIO_PinAltFunMode = 5;   /* AF5 = SPI1 on PA5/PA6/PA7 */
	SPIPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = SD_SCK_PIN;
	GPIO_Init(&SPIPins);

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = SD_MISO_PIN;
	GPIO_Init(&SPIPins);

	SPIPins.GPIO_PinConfig.GPIO_PinNumber = SD_MOSI_PIN;
	GPIO_Init(&SPIPins);

	/* CS is a plain GPIO output, not AF -- driven manually by the SD driver. */
	CSPin.pGPIOx = SD_SPI_PORT;
	CSPin.GPIO_PinConfig.GPIO_PinNumber = SD_CS_PIN;
	CSPin.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	CSPin.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	CSPin.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	CSPin.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;

	GPIO_Init(&CSPin);
	GPIO_WriteToOutputPin(SD_SPI_PORT, SD_CS_PIN, 1U);   /* deselected by default */
}

static void SD_SPI1_Inits(void)
{
	SPI1Handle.pSPIx = SPI1;
	SPI1Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI1Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI1Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI1Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI1Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	/* Slow prescaler for the SD init sequence (spec requires <=400kHz-ish);
	 * SD_Init() switches to a faster prescaler once the card is ready. */
	SPI1Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV256;
	SPI1Handle.SPIConfig.SPI_SSM = SPI_SSM_EN;

	SPI_Init(&SPI1Handle);
	SPI_SSIConfig(SPI1, ENABLE);       /* software NSS management, keep internal NSS high */
	SPI_PeripheralControl(SPI1, ENABLE);
}

/* Parses a signed decimal integer from a NUL-terminated string. Returns 0 on malformed input. */
static uint8_t ParseInt32(const char *text, int32_t *out)
{
	int32_t sign = 1;
	int32_t value = 0;
	uint8_t has_digit = 0;

	if (*text == '-')
	{
		sign = -1;
		text++;
	}

	while (*text != '\0')
	{
		if ((*text < '0') || (*text > '9'))
		{
			return 0;
		}

		value = (value * 10) + (int32_t)(*text - '0');
		has_digit = 1;
		text++;
	}

	if (!has_digit)
	{
		return 0;
	}

	*out = value * sign;
	return 1;
}

/*
 * Enables the Cortex-M4F hardware FPU (CP10/CP11 full access via CPACR).
 * This project never used 'float' before this app, so CPACR was never touched
 * by any startup code. Without this, the first float instruction (in the HTS221
 * calibration conversion) raises a UsageFault, which falls through to the weak
 * Default_Handler in the startup .s file -- an empty infinite loop, no recovery.
 * Must run before any floating-point instruction executes.
 */
static void FPU_Enable(void)
{
	volatile uint32_t *cpacr = (volatile uint32_t *)0xE000ED88UL;

	*cpacr |= (0xFUL << 20);   /* full access to CP10 and CP11 */

	__asm volatile ("dsb");
	__asm volatile ("isb");
}

int main(void)
{
	uint8_t rx;
	uint8_t cmd_buffer[CMD_BUFFER_SIZE];
	uint32_t cmd_index = 0;

	FPU_Enable();

	SystemClock_HSI_Init();
	HSI16_ENABLE();

	USART1_CCLK_HSI16();
	I2C2_CCLK_HSI16();
	I2C1_CCLK_HSI16();

	USART1_GPIOInits();
	I2C2_GPIOInits();
	I2C1_GPIOInits();
	SD_SPI1_GPIOInits();
	GPIO_LEDInit();
	GPIO_FanInit();

	USART1_Inits();
	I2C2_Inits();
	I2C1_Inits();
	SD_SPI1_Inits();
	RingBuffer_Init(&usart_rx_buffer, rx_storage, RX_BUFFER_SIZE);
	USART1_RXInterruptEnable();

	sdHandle.pSPIx = SPI1;
	sdHandle.pCSPort = SD_SPI_PORT;
	sdHandle.CSPin = SD_CS_PIN;
	sdHandle.CardType = SD_CARD_TYPE_UNKNOWN;

	SendString("READY ENV_MONITOR\r\n");

	{
		uint8_t init_status = Sensors_Init();
		SendString("SENSORS_INIT STATUS=");
		SendStatus(init_status);
		SendString("\r\n");
	}

	while (1)
	{
		uint32_t primask = IRQ_SaveAndDisable();
		uint32_t errors = usart_error_events;
		usart_error_events = USART_ERROR_NONE;
		IRQ_Restore(primask);

		if (errors != USART_ERROR_NONE)
		{
			SendString("USART_ERROR=");
			SendHex8((uint8_t)errors);
			SendString("\r\n");
			USART_ClearErrorCode(&USART1Handle);
		}

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

static uint8_t I2C_ReadReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t *value)
{
	uint8_t status;

	status = I2C_MasterSendData(&I2C2Handle, &reg_addr, 1U, slave_addr, I2C_SR_ENABLE);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	return I2C_MasterReceiveData(&I2C2Handle, value, 1U, slave_addr, I2C_SR_DISABLE);
}

static uint8_t I2C_WriteReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t value)
{
	uint8_t data[2];

	data[0] = reg_addr;
	data[1] = value;

	return I2C_MasterSendData(&I2C2Handle, data, 2U, slave_addr, I2C_SR_DISABLE);
}

/*
 * Same write-reg-then-repeated-START-read pattern as I2C_ReadReg8(), but on
 * I2C1Handle. Used by "I2C1 READX" against whatever module is wired to the
 * exposed I2C1 pins (PB8/PB9, ARDUINO D15/D14) -- generic on purpose, since
 * we don't necessarily know the register map of an unlabeled breakout board.
 */
static uint8_t I2C1_ReadReg8(uint8_t slave_addr, uint8_t reg_addr, uint8_t *value)
{
	uint8_t status;

	status = I2C_MasterSendData(&I2C1Handle, &reg_addr, 1U, slave_addr, I2C_SR_ENABLE);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	return I2C_MasterReceiveData(&I2C1Handle, value, 1U, slave_addr, I2C_SR_DISABLE);
}

/*
 * Parses one or two hex digits starting at *text, advancing *text past what
 * it consumed. Returns 0 (and leaves *text and *out untouched) if there is
 * no valid hex digit at the current position.
 */
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

/* Parses "<hex byte> <hex byte>" (any amount of whitespace between/around) into a and b. */
static uint8_t ParseHex8Pair(const char *text, uint8_t *a, uint8_t *b)
{
	while (*text == ' ')
	{
		text++;
	}

	if (!ParseHexByte(&text, a))
	{
		return 0;
	}

	while (*text == ' ')
	{
		text++;
	}

	if (!ParseHexByte(&text, b))
	{
		return 0;
	}

	return 1;
}

static uint8_t Sensor_ReadId(uint8_t slave_addr, uint8_t *id)
{
	return I2C_ReadReg8(slave_addr, LPS22HB_WHO_AM_I_REG, id);
}

static uint8_t Sensors_Init(void)
{
	uint8_t status;

	/*
	 * First version: power/configure the sensors enough for raw register reads.
	 * Physical-unit conversion can be added after hardware validation.
	 */
	status = I2C_WriteReg8(LPS22HB_ADDR, LPS22HB_CTRL_REG1, 0x50U);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = I2C_WriteReg8(HTS221_ADDR, HTS221_CTRL_REG1, 0x85U);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = HTS221_ReadCalibration(&hts221_calib);
	if (status != I2C_ERROR_NONE)
	{
		hts221_calib_valid = 0;
		return status;
	}

	hts221_calib_valid = 1;

	return I2C_ERROR_NONE;
}

static uint8_t LPS22HB_ReadPressureRaw(int32_t *pressure_raw)
{
	uint8_t xl = 0;
	uint8_t l = 0;
	uint8_t h = 0;
	uint8_t status;
	int32_t raw24;

	status = I2C_ReadReg8(LPS22HB_ADDR, LPS22HB_PRESS_OUT_XL, &xl);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = I2C_ReadReg8(LPS22HB_ADDR, LPS22HB_PRESS_OUT_L, &l);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = I2C_ReadReg8(LPS22HB_ADDR, LPS22HB_PRESS_OUT_H, &h);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	raw24 = ((int32_t)h << 16) | ((int32_t)l << 8) | (int32_t)xl;
	if (raw24 & 0x00800000L)
	{
		raw24 |= 0xFF000000L;
	}

	*pressure_raw = raw24;
	return I2C_ERROR_NONE;
}

static uint8_t HTS221_ReadTemperatureRaw(int16_t *temperature_raw)
{
	uint8_t l = 0;
	uint8_t h = 0;
	uint8_t status;

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_TEMP_OUT_L, &l);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_TEMP_OUT_H, &h);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	*temperature_raw = (int16_t)(((uint16_t)h << 8) | l);
	return I2C_ERROR_NONE;
}

static uint8_t HTS221_ReadHumidityRaw(int16_t *humidity_raw)
{
	uint8_t l = 0;
	uint8_t h = 0;
	uint8_t status;

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_HUMIDITY_OUT_L, &l);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_HUMIDITY_OUT_H, &h);
	if (status != I2C_ERROR_NONE)
	{
		return status;
	}

	*humidity_raw = (int16_t)(((uint16_t)h << 8) | l);
	return I2C_ERROR_NONE;
}

/*
 * Reads the HTS221 factory calibration block once and fills 'calib' with values
 * already converted to real units (degC, %RH) so every later conversion is just
 * the two-point linear interpolation, no bit unpacking anywhere else.
 *
 * Register map (ST HTS221 datasheet):
 *   H0_rH_x2 / H1_rH_x2   -> humidity reference points, stored x2   (0x30 / 0x31)
 *   T0_degC_x8 / T1_degC_x8 -> temperature reference points, x8, low 8 bits (0x32 / 0x33)
 *   T1_T0_MSB             -> extra 2 high bits for each temperature point (0x35)
 *                            bits[1:0] = T0_degC_x8[9:8], bits[3:2] = T1_degC_x8[9:8]
 *   H0_OUT / H1_OUT        -> raw ADC counts measured at the humidity reference points
 *   T0_OUT / T1_OUT        -> raw ADC counts measured at the temperature reference points
 */
static uint8_t HTS221_ReadCalibration(HTS221_Calib_t *calib)
{
	uint8_t status;
	uint8_t h0_rh_x2 = 0, h1_rh_x2 = 0;
	uint8_t t0_degc_x8_l = 0, t1_degc_x8_l = 0;
	uint8_t t1_t0_msb = 0;
	uint8_t h0_out_l = 0, h0_out_h = 0;
	uint8_t h1_out_l = 0, h1_out_h = 0;
	uint8_t t0_out_l = 0, t0_out_h = 0;
	uint8_t t1_out_l = 0, t1_out_h = 0;
	uint16_t t0_degc_x8_full;
	uint16_t t1_degc_x8_full;

	SendString("CALIB:START\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H0_RH_X2, &h0_rh_x2);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H0_RH\r\n"); return status; }
	SendString("CALIB:H0_RH OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H1_RH_X2, &h1_rh_x2);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H1_RH\r\n"); return status; }
	SendString("CALIB:H1_RH OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T0_DEGC_X8, &t0_degc_x8_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T0_DEGC\r\n"); return status; }
	SendString("CALIB:T0_DEGC OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T1_DEGC_X8, &t1_degc_x8_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T1_DEGC\r\n"); return status; }
	SendString("CALIB:T1_DEGC OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T1_T0_MSB, &t1_t0_msb);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T1_T0_MSB\r\n"); return status; }
	SendString("CALIB:T1_T0_MSB OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H0_OUT_L, &h0_out_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H0_OUT_L\r\n"); return status; }
	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H0_OUT_H, &h0_out_h);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H0_OUT_H\r\n"); return status; }
	SendString("CALIB:H0_OUT OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H1_OUT_L, &h1_out_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H1_OUT_L\r\n"); return status; }
	status = I2C_ReadReg8(HTS221_ADDR, HTS221_H1_OUT_H, &h1_out_h);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL H1_OUT_H\r\n"); return status; }
	SendString("CALIB:H1_OUT OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T0_OUT_L, &t0_out_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T0_OUT_L\r\n"); return status; }
	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T0_OUT_H, &t0_out_h);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T0_OUT_H\r\n"); return status; }
	SendString("CALIB:T0_OUT OK\r\n");

	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T1_OUT_L, &t1_out_l);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T1_OUT_L\r\n"); return status; }
	status = I2C_ReadReg8(HTS221_ADDR, HTS221_T1_OUT_H, &t1_out_h);
	if (status != I2C_ERROR_NONE) { SendString("CALIB:FAIL T1_OUT_H\r\n"); return status; }
	SendString("CALIB:T1_OUT OK\r\n");

	SendString("CALIB:DONE\r\n");

	/* Stitch the 2 high bits from T1_T0_MSB onto the 8 low bits of each x8 value. */
	t0_degc_x8_full = ((uint16_t)(t1_t0_msb & 0x03U) << 8) | (uint16_t)t0_degc_x8_l;
	t1_degc_x8_full = ((uint16_t)((t1_t0_msb >> 2) & 0x03U) << 8) | (uint16_t)t1_degc_x8_l;

	calib->T0_degC = (float)t0_degc_x8_full / 8.0f;
	calib->T1_degC = (float)t1_degc_x8_full / 8.0f;
	calib->H0_rH   = (float)h0_rh_x2 / 2.0f;
	calib->H1_rH   = (float)h1_rh_x2 / 2.0f;

	calib->H0_OUT = (int16_t)(((uint16_t)h0_out_h << 8) | h0_out_l);
	calib->H1_OUT = (int16_t)(((uint16_t)h1_out_h << 8) | h1_out_l);
	calib->T0_OUT = (int16_t)(((uint16_t)t0_out_h << 8) | t0_out_l);
	calib->T1_OUT = (int16_t)(((uint16_t)t1_out_h << 8) | t1_out_l);

	return I2C_ERROR_NONE;
}

/*
 * LPS22HB has no factory two-point calibration to read: the datasheet gives a fixed
 * sensitivity of 4096 LSB per hPa, so the raw 24-bit reading converts directly.
 * Returned value is hPa * 10 (one implied decimal digit) so it can travel through
 * SendInt32() without needing float printing.
 */
static int32_t LPS22HB_ConvertPressureHPa_x10(int32_t pressure_raw)
{
	return (int32_t)(((int64_t)pressure_raw * 10) / 4096);
}

/*
 * Two-point linear interpolation: T_degC = T0_degC + (T_OUT - T0_OUT) * (T1_degC - T0_degC) / (T1_OUT - T0_OUT)
 * Returns degC * 10 for the same reason as above (integer-friendly for SendInt32).
 */
static int32_t HTS221_ConvertTemperatureC_x10(int16_t temperature_raw, const HTS221_Calib_t *calib)
{
	float t_degc;

	t_degc = calib->T0_degC + ((float)(temperature_raw - calib->T0_OUT) *
	         (calib->T1_degC - calib->T0_degC)) / (float)(calib->T1_OUT - calib->T0_OUT);

	return (int32_t)(t_degc * 10.0f);
}

/* Same interpolation, humidity side. Returns %RH * 10. */
static int32_t HTS221_ConvertHumidityRH_x10(int16_t humidity_raw, const HTS221_Calib_t *calib)
{
	float h_rh;

	h_rh = calib->H0_rH + ((float)(humidity_raw - calib->H0_OUT) *
	       (calib->H1_rH - calib->H0_rH)) / (float)(calib->H1_OUT - calib->H0_OUT);

	return (int32_t)(h_rh * 10.0f);
}

static void ProcessCommand(uint8_t *cmd)
{
	if (strcmp((char *)cmd, "PING") == 0)
	{
		SendString("PONG\r\n");
	}
	else if (strcmp((char *)cmd, "HELP") == 0)
	{
		SendString("CMDS:PING,APP_ID?,SENSOR IDS,INIT SENSORS,PRESSURE RAW,TEMP RAW,HUM RAW,MONITOR,"
		           "PRESSURE HPA,TEMP C,HUM RH,MONITOR FULL,THRESHOLD <x10>,THRESHOLD?,"
		           "FAN STATUS,SD INIT,SD STATUS,SD WRITE TEST,SD READ TEST,USART STATUS,"
		           "I2C1 SCAN,I2C1 READX <addr> <reg>\r\n");
	}
	else if (strcmp((char *)cmd, "SENSOR IDS") == 0)
	{
		uint8_t lps_id = 0;
		uint8_t hts_id = 0;
		uint8_t lps_status = Sensor_ReadId(LPS22HB_ADDR, &lps_id);
		uint8_t hts_status = Sensor_ReadId(HTS221_ADDR, &hts_id);

		SendString("LPS22HB=");
		SendHex8(lps_id);
		SendString(" HTS221=");
		SendHex8(hts_id);
		SendString(" STATUS=");
		if ((lps_status == I2C_ERROR_NONE) && (hts_status == I2C_ERROR_NONE) &&
		    (lps_id == LPS22HB_WHO_AM_I_VAL) && (hts_id == HTS221_WHO_AM_I_VAL))
		{
			SendString("OK\r\n");
		}
		else
		{
			SendString("ERROR\r\n");
		}
	}
	else if (strcmp((char *)cmd, "INIT SENSORS") == 0)
	{
		uint8_t status = Sensors_Init();

		SendString("INIT STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "PRESSURE RAW") == 0)
	{
		int32_t pressure_raw = 0;
		uint8_t status = LPS22HB_ReadPressureRaw(&pressure_raw);

		SendString("PRESS_RAW=");
		SendInt32(pressure_raw);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "TEMP RAW") == 0)
	{
		int16_t temperature_raw = 0;
		uint8_t status = HTS221_ReadTemperatureRaw(&temperature_raw);

		SendString("TEMP_RAW=");
		SendInt32((int32_t)temperature_raw);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "HUM RAW") == 0)
	{
		int16_t humidity_raw = 0;
		uint8_t status = HTS221_ReadHumidityRaw(&humidity_raw);

		SendString("HUM_RAW=");
		SendInt32((int32_t)humidity_raw);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "MONITOR") == 0)
	{
		int32_t pressure_raw = 0;
		int16_t temperature_raw = 0;
		int16_t humidity_raw = 0;
		uint8_t pressure_status = LPS22HB_ReadPressureRaw(&pressure_raw);
		uint8_t temp_status = HTS221_ReadTemperatureRaw(&temperature_raw);
		uint8_t hum_status = HTS221_ReadHumidityRaw(&humidity_raw);

		SendString("PRESS_RAW=");
		SendInt32(pressure_raw);
		SendString(" TEMP_RAW=");
		SendInt32((int32_t)temperature_raw);
		SendString(" HUM_RAW=");
		SendInt32((int32_t)humidity_raw);
		SendString(" STATUS=");
		if ((pressure_status == I2C_ERROR_NONE) && (temp_status == I2C_ERROR_NONE) &&
		    (hum_status == I2C_ERROR_NONE))
		{
			SendString("OK\r\n");
		}
		else
		{
			SendString("ERROR\r\n");
		}
	}
	else if (strcmp((char *)cmd, "PRESSURE HPA") == 0)
	{
		int32_t pressure_raw = 0;
		uint8_t status = LPS22HB_ReadPressureRaw(&pressure_raw);
		int32_t pressure_hpa_x10 = LPS22HB_ConvertPressureHPa_x10(pressure_raw);

		SendString("PRESS_HPA_X10=");
		SendInt32(pressure_hpa_x10);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "TEMP C") == 0)
	{
		int16_t temperature_raw = 0;
		uint8_t status = HTS221_ReadTemperatureRaw(&temperature_raw);
		int32_t temp_c_x10 = 0;

		if (hts221_calib_valid)
		{
			temp_c_x10 = HTS221_ConvertTemperatureC_x10(temperature_raw, &hts221_calib);
		}
		else
		{
			status = (uint8_t)I2C_ERROR_NACK;
		}

		SendString("TEMP_C_X10=");
		SendInt32(temp_c_x10);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "HUM RH") == 0)
	{
		int16_t humidity_raw = 0;
		uint8_t status = HTS221_ReadHumidityRaw(&humidity_raw);
		int32_t hum_rh_x10 = 0;

		if (hts221_calib_valid)
		{
			hum_rh_x10 = HTS221_ConvertHumidityRH_x10(humidity_raw, &hts221_calib);
			Fan_UpdateState(hum_rh_x10);
		}
		else
		{
			status = (uint8_t)I2C_ERROR_NACK;
		}

		SendString("HUM_RH_X10=");
		SendInt32(hum_rh_x10);
		SendString(" STATUS=");
		SendStatus(status);
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "MONITOR FULL") == 0)
	{
		int32_t pressure_raw = 0;
		int16_t temperature_raw = 0;
		int16_t humidity_raw = 0;
		uint8_t pressure_status = LPS22HB_ReadPressureRaw(&pressure_raw);
		uint8_t temp_status = HTS221_ReadTemperatureRaw(&temperature_raw);
		uint8_t hum_status = HTS221_ReadHumidityRaw(&humidity_raw);
		int32_t pressure_hpa_x10 = LPS22HB_ConvertPressureHPa_x10(pressure_raw);
		int32_t temp_c_x10 = hts221_calib_valid ? HTS221_ConvertTemperatureC_x10(temperature_raw, &hts221_calib) : 0;
		int32_t hum_rh_x10 = hts221_calib_valid ? HTS221_ConvertHumidityRH_x10(humidity_raw, &hts221_calib) : 0;

		if (hts221_calib_valid)
		{
			Fan_UpdateState(hum_rh_x10);
		}

		SendString("PRESS_HPA_X10=");
		SendInt32(pressure_hpa_x10);
		SendString(" TEMP_C_X10=");
		SendInt32(temp_c_x10);
		SendString(" HUM_RH_X10=");
		SendInt32(hum_rh_x10);
		SendString(" STATUS=");
		if ((pressure_status == I2C_ERROR_NONE) && (temp_status == I2C_ERROR_NONE) &&
		    (hum_status == I2C_ERROR_NONE) && (hts221_calib_valid != 0U))
		{
			SendString("OK\r\n");
		}
		else
		{
			SendString("ERROR\r\n");
		}
	}
	else if (strncmp((char *)cmd, "THRESHOLD ", 10) == 0)
	{
		int32_t value = 0;

		if (ParseInt32((char *)cmd + 10, &value))
		{
			humidity_threshold_x10 = value;
			SendString("THRESHOLD_X10=");
			SendInt32(humidity_threshold_x10);
			SendString(" STATUS=OK\r\n");
		}
		else
		{
			SendString("ERR:BAD_ARG\r\n");
		}
	}
	else if (strcmp((char *)cmd, "THRESHOLD?") == 0)
	{
		SendString("THRESHOLD_X10=");
		SendInt32(humidity_threshold_x10);
		SendString(" STATUS=OK\r\n");
	}
	else if (strcmp((char *)cmd, "FAN STATUS") == 0)
	{
		SendString("FAN=");
		SendString(fan_state ? "ON" : "OFF");
		SendString(" THRESHOLD_X10=");
		SendInt32(humidity_threshold_x10);
		SendString(" STATUS=OK\r\n");
	}
	else if (strcmp((char *)cmd, "SD INIT") == 0)
	{
		uint8_t status = SD_Init(&sdHandle);

		sd_init_done = (status == SD_OK) ? 1U : 0U;

		SendString("SD_INIT STATUS=");
		SendSDStatus(status);
		SendString(" TYPE=");
		if (status == SD_OK)
		{
			SendString((sdHandle.CardType == SD_CARD_TYPE_SDHC_SDXC) ? "SDHC_SDXC" : "SDSC");
		}
		else
		{
			SendString("UNKNOWN");
		}
		SendString("\r\n");
	}
	else if (strcmp((char *)cmd, "SD STATUS") == 0)
	{
		SendString("SD_READY=");
		SendString(sd_init_done ? "1" : "0");
		SendString(" TYPE=");
		if (sd_init_done)
		{
			SendString((sdHandle.CardType == SD_CARD_TYPE_SDHC_SDXC) ? "SDHC_SDXC" : "SDSC");
		}
		else
		{
			SendString("UNKNOWN");
		}
		SendString(" STATUS=OK\r\n");
	}
	else if (strcmp((char *)cmd, "SD WRITE TEST") == 0)
	{
		if (!sd_init_done)
		{
			SendString("ERR:SD_NOT_INIT\r\n");
		}
		else
		{
			uint32_t i;
			uint8_t status;

			for (i = 0; i < SD_BLOCK_SIZE; i++)
			{
				sd_test_buffer[i] = (uint8_t)(i & 0xFFU);
			}

			status = SD_WriteBlock(&sdHandle, SD_TEST_BLOCK_ADDR, sd_test_buffer);

			SendString("SD_WRITE_TEST STATUS=");
			SendSDStatus(status);
			SendString("\r\n");
		}
	}
	else if (strcmp((char *)cmd, "SD READ TEST") == 0)
	{
		if (!sd_init_done)
		{
			SendString("ERR:SD_NOT_INIT\r\n");
		}
		else
		{
			uint32_t i;
			uint8_t status;
			uint8_t match = 1U;

			status = SD_ReadBlock(&sdHandle, SD_TEST_BLOCK_ADDR, sd_test_buffer);

			if (status == SD_OK)
			{
				for (i = 0; i < SD_BLOCK_SIZE; i++)
				{
					if (sd_test_buffer[i] != (uint8_t)(i & 0xFFU))
					{
						match = 0U;
						break;
					}
				}
			}
			else
			{
				match = 0U;
			}

			SendString("SD_READ_TEST STATUS=");
			SendSDStatus(status);
			SendString(" MATCH=");
			SendString(match ? "1" : "0");
			SendString("\r\n");
		}
	}
	else if (strcmp((char *)cmd, "USART STATUS") == 0)
	{
		SendUSARTStatus();
	}
	else if (strcmp((char *)cmd, "I2C1 SCAN") == 0)
	{
		/*
		 * Standard I2C bus scan: try a 1-byte write (register-pointer set,
		 * no actual value write -- same first phase as any register read)
		 * against every valid 7-bit address and see which ones ACK.
		 * NBYTES=0 probes aren't used here because this driver's blocking
		 * send only checks NACKF from inside the byte-transmit wait loop;
		 * with Len=0 that loop never runs, so a NACKed address would hang
		 * forever instead of reporting NACK. Len=1 sidesteps that.
		 */
		uint8_t addr;
		uint8_t found = 0U;

		SendString("I2C1_SCAN:\r\n");

		for (addr = 0x08U; addr <= 0x77U; addr++)
		{
			uint8_t dummy = 0x00U;
			uint8_t status = I2C_MasterSendData(&I2C1Handle, &dummy, 1U, addr, I2C_SR_DISABLE);

			if (status == I2C_ERROR_NONE)
			{
				SendString("  ADDR=");
				SendHex8(addr);
				SendString("\r\n");
				found = 1U;
			}
		}

		SendString("I2C1_SCAN_DONE FOUND=");
		SendString(found ? "1" : "0");
		SendString("\r\n");
	}
	else if (strncmp((char *)cmd, "I2C1 READX ", 11) == 0)
	{
		uint8_t addr = 0;
		uint8_t reg = 0;

		if (ParseHex8Pair((char *)cmd + 11, &addr, &reg))
		{
			uint8_t value = 0;
			uint8_t status = I2C1_ReadReg8(addr, reg, &value);

			SendString("I2C1_READX ADDR=");
			SendHex8(addr);
			SendString(" REG=");
			SendHex8(reg);
			SendString(" VAL=");
			SendHex8(value);
			SendString(" STATUS=");
			SendStatus(status);
			SendString("\r\n");
		}
		else
		{
			SendString("ERR:BAD_ARG\r\n");
		}
	}
	else if (strcmp((char *)cmd, "APP_ID?") == 0)
	{
		SendString("ENV_MONITOR_USART\r\n");
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

static void SendInt32(int32_t value)
{
	char buffer[12];
	uint32_t index = 0;
	uint32_t number;

	if (value == 0)
	{
		SendString("0");
		return;
	}

	if (value < 0)
	{
		SendString("-");
		number = (uint32_t)(-value);
	}
	else
	{
		number = (uint32_t)value;
	}

	while ((number > 0U) && (index < sizeof(buffer)))
	{
		buffer[index++] = (char)('0' + (number % 10U));
		number /= 10U;
	}

	while (index > 0U)
	{
		index--;
		USART_SendData(&USART1Handle, (uint8_t *)&buffer[index], 1U);
	}
}

static void SendStatus(uint8_t status)
{
	if (status == I2C_ERROR_NONE)
	{
		SendString("OK");
	}
	else if (status == (uint8_t)I2C_ERROR_NACK)
	{
		SendString("I2C_NACK");
	}
	else if (status == (uint8_t)I2C_ERROR_TIMEOUT)
	{
		SendString("I2C_TIMEOUT");
	}
	else
	{
		SendString("ERROR");
	}
}

static void SendSDStatus(uint8_t status)
{
	switch (status)
	{
		case SD_OK:                SendString("OK"); break;
		case SD_ERROR_NO_CARD:      SendString("NO_CARD"); break;
		case SD_ERROR_UNSUPPORTED:  SendString("UNSUPPORTED"); break;
		case SD_ERROR_VOLTAGE:      SendString("VOLTAGE"); break;
		case SD_TIMEOUT:            SendString("TIMEOUT"); break;
		default:                    SendString("ERROR"); break;
	}
}

static void SendUSARTStatus(void)
{
	SendString("ISR=");
	SendHex8((uint8_t)(USART1Handle.pUSARTx->ISR & 0xFFU));
	SendString(" ERR=");
	SendHex8((uint8_t)(USART_GetErrorCode(&USART1Handle) & 0xFFU));
	SendString("\r\n");
}

static uint32_t IRQ_SaveAndDisable(void)
{
	uint32_t primask;

	__asm volatile ("MRS %0, PRIMASK" : "=r" (primask) :: "memory");
	__asm volatile ("CPSID i" ::: "memory");

	return primask;
}

static void IRQ_Restore(uint32_t primask)
{
	__asm volatile ("MSR PRIMASK, %0" :: "r" (primask) : "memory");
}

void USART1_IRQHandler(void)
{
	USART_IRQHandling(&USART1Handle);
}

void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t AppEv)
{
	if (AppEv == USART_EVENT_RXNE)
	{
		RingBuffer_Write(&usart_rx_buffer, pUSARTHandle->RxByte);
	}
	else if (AppEv == USART_EVENT_ERR_PE)
	{
		usart_error_events |= USART_ERROR_PE;
	}
	else if (AppEv == USART_EVENT_ERR_FE)
	{
		usart_error_events |= USART_ERROR_FE;
	}
	else if (AppEv == USART_EVENT_ERR_NE)
	{
		usart_error_events |= USART_ERROR_NE;
	}
	else if (AppEv == USART_EVENT_ERR_ORE)
	{
		usart_error_events |= USART_ERROR_ORE;
	}
}

