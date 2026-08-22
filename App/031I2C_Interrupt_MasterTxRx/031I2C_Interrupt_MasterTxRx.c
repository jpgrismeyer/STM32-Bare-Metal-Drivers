/*
 * 031I2C_Interrupt_MasterTxRx.c
 *
 * I2C register read (write reg-address, repeated START, read data) done entirely
 * through I2C_MasterSendDataIT()/I2C_MasterReceiveDataIT() -- no blocking while loop
 * anywhere in the transfer path. Compare against 013I2C_MasterRx_Sensor (polling) and
 * 030Environmental_Monitor_USART's I2C_ReadReg8() (also polling): same register-read
 * pattern, now driven entirely from I2C_EV_IRQHandling()/I2C_ER_IRQHandling() instead of
 * busy-waiting on ISR flags in the application.
 *
 * Target: onboard LPS22HB pressure sensor (I2C2, address 0x5D), reading WHO_AM_I (0x0F),
 * expected value 0xB1 -- same known-good target used throughout this repo.
 */

#include "stm32l47xx.h"
#include "ring_buffer.h"
#include <string.h>

#define MY_ADDR                 0x61

#define LPS22HB_ADDR            0x5D
#define LPS22HB_WHO_AM_I_REG    0x0F
#define LPS22HB_WHO_AM_I_VAL    0xB1

#define CMD_BUFFER_SIZE         64
#define RX_BUFFER_SIZE          128

USART_Handle_t USART1Handle;
I2C_Handle_t I2C2Handle;

static volatile uint8_t rx_storage[RX_BUFFER_SIZE];
static RingBuffer_t usart_rx_buffer;

/* IT transfer state, updated from I2C_ApplicationEventCallBack() (interrupt context)
 * and read from the main loop -- volatile because of that shared-state pattern, same
 * reasoning as usart_error_events in App 030. */
static volatile uint8_t  it_reg_addr;
static volatile uint8_t  it_who_am_i;
static volatile uint8_t  it_last_event;
static volatile uint8_t  it_transfer_active;

static void USART1_GPIOInits(void);
static void USART1_Inits(void);
static void USART1_RXInterruptEnable(void);
static void I2C2_GPIOInits(void);
static void I2C2_Inits(void);
static void I2C2_IRQInterruptEnable(void);

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

static void I2C2_IRQInterruptEnable(void)
{
	I2C_IRQPriorityConfig(IRQ_NO_I2C2_EV, NVIC_IRQ_PRI15);
	I2C_IRQInterruptConfig(IRQ_NO_I2C2_EV, ENABLE);

	I2C_IRQPriorityConfig(IRQ_NO_I2C2_ER, NVIC_IRQ_PRI15);
	I2C_IRQInterruptConfig(IRQ_NO_I2C2_ER, ENABLE);
}

int main(void)
{
	uint8_t rx;
	uint8_t cmd_buffer[CMD_BUFFER_SIZE];
	uint32_t cmd_index = 0;

	SystemClock_HSI_Init();
	HSI16_ENABLE();

	USART1_CCLK_HSI16();
	I2C2_CCLK_HSI16();

	USART1_GPIOInits();
	I2C2_GPIOInits();

	USART1_Inits();
	I2C2_Inits();
	RingBuffer_Init(&usart_rx_buffer, rx_storage, RX_BUFFER_SIZE);
	USART1_RXInterruptEnable();
	I2C2_IRQInterruptEnable();

	SendString("READY I2C_IT\r\n");

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
		SendString("CMDS:PING,APP_ID?,I2C WHOAMI IT,I2C STATUS\r\n");
	}
	else if (strcmp((char *)cmd, "APP_ID?") == 0)
	{
		SendString("I2C_INTERRUPT_MASTERTXRX\r\n");
	}
	else if (strcmp((char *)cmd, "I2C WHOAMI IT") == 0)
	{
		if (I2C2Handle.TxRxState != I2C_READY)
		{
			SendString("ERR:I2C_BUSY\r\n");
		}
		else
		{
			uint8_t status;

			it_reg_addr = LPS22HB_WHO_AM_I_REG;
			it_transfer_active = 1U;
			it_last_event = 0U;

			/* Write the register address with Sr=ENABLE (repeated START, bus stays
			 * held) -- I2C_EVENT_TX_CMPLT fires from the ISR once that write lands,
			 * and the callback below chains the read automatically. */
			status = I2C_MasterSendDataIT(&I2C2Handle, (uint8_t *)&it_reg_addr, 1U,
			                               LPS22HB_ADDR, I2C_SR_ENABLE);

			if (status != I2C_ERROR_NONE)
			{
				it_transfer_active = 0U;
				SendString("ERR:I2C_START_FAILED\r\n");
			}
			else
			{
				SendString("I2C_WHOAMI_IT STARTED\r\n");
			}
		}
	}
	else if (strcmp((char *)cmd, "I2C STATUS") == 0)
	{
		SendString("STATE=");
		if (I2C2Handle.TxRxState == I2C_READY)
		{
			SendString("READY");
		}
		else if (I2C2Handle.TxRxState == I2C_BUSY_IN_TX)
		{
			SendString("BUSY_TX");
		}
		else
		{
			SendString("BUSY_RX");
		}
		SendString(" LAST_EVENT=");
		SendHex8(it_last_event);
		SendString(" WHO_AM_I=");
		SendHex8(it_who_am_i);
		SendString(" MATCH=");
		SendString((it_who_am_i == LPS22HB_WHO_AM_I_VAL) ? "1" : "0");
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

void I2C2_EV_IRQHandler(void)
{
	I2C_EV_IRQHandling(&I2C2Handle);
}

void I2C2_ER_IRQHandler(void)
{
	I2C_ER_IRQHandling(&I2C2Handle);
}

void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t AppEv)
{
	if (AppEv == USART_EVENT_RXNE)
	{
		RingBuffer_Write(&usart_rx_buffer, pUSARTHandle->RxByte);
	}
}

/*
 * This is the payoff: no while loop anywhere in this file waits on an I2C flag. Every
 * step of the register-read sequence (write reg address -> repeated START -> read data
 * -> STOP) is driven by hardware events landing here.
 */
void I2C_ApplicationEventCallBack(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
	it_last_event = AppEv;

	if (AppEv == I2C_EVENT_TX_CMPLT)
	{
		/* The register-address write finished with the bus still held (repeated
		 * START). Chain the 1-byte read of WHO_AM_I, Sr=DISABLE this time so the
		 * transfer closes with a real STOP. */
		(void)I2C_MasterReceiveDataIT(pI2CHandle, (uint8_t *)&it_who_am_i, 1U,
		                               LPS22HB_ADDR, I2C_SR_DISABLE);
	}
	else if (AppEv == I2C_EVENT_RX_CMPLT)
	{
		it_transfer_active = 0U;
	}
	else if ((AppEv == I2C_ERROR_EVENT_NACK) || (AppEv == I2C_ERROR_EVENT_BERR) ||
	         (AppEv == I2C_ERROR_EVENT_ARLO) || (AppEv == I2C_ERROR_EVENT_OVR))
	{
		it_transfer_active = 0U;
	}
}
