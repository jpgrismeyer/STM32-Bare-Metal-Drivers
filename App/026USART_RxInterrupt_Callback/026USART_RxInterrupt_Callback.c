#include "stm32l47xx.h"
#include "ring_buffer.h"
#include <string.h>

#define CMD_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 128

USART_Handle_t USART1Handle;

static volatile uint8_t rx_storage[RX_BUFFER_SIZE];
static RingBuffer_t usart_rx_buffer;

void ProcessCommand(uint8_t *cmd);

//Initialize PB6 and PB7 as USART pins.
void USART1_GPIOInits(void)
{
	GPIO_Handle_t USARTPins;

	USARTPins.pGPIOx = GPIOB;
	USARTPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	USARTPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	USARTPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	USARTPins.GPIO_PinConfig.GPIO_PinAltFunMode = 7;
	USARTPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	//USART1 TX
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&USARTPins);

	//USART1 RX
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&USARTPins);
}

//Initialize USART1 with specific parameters.
void USART1_Inits(void)
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

	USART_IRQPriorityConfig(IRQ_NO_USART1, NVIC_IRQ_PRI15);
	USART_IRQInterruptConfig(IRQ_NO_USART1, ENABLE);
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
	USART1_Inits();
	RingBuffer_Init(&usart_rx_buffer, rx_storage, RX_BUFFER_SIZE);
	USART1_RXInterruptEnable();

	uint8_t ready[] = "READY\r\n";
	USART_SendData(&USART1Handle, ready, sizeof(ready) - 1);

	while (1)
	{
		if (RingBuffer_Read(&usart_rx_buffer, &rx))
		{
			if (rx == '\r' || rx == '\n')
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
				if (cmd_index < CMD_BUFFER_SIZE - 1U)
				{
					cmd_buffer[cmd_index++] = rx;
				}
				else
				{
					cmd_index = 0;
					uint8_t response[] = "ERR:CMD_TOO_LONG\r\n";
					USART_SendData(&USART1Handle, response, sizeof(response) - 1);
				}
			}
		}
	}
}

void ProcessCommand(uint8_t *cmd)
{
	if (strcmp((char *)cmd, "PING") == 0)
	{
		uint8_t response[] = "PONG\r\n";
		USART_SendData(&USART1Handle, response, sizeof(response) - 1);
	}
	else if (strncmp((char *)cmd, "ECHO ", 5) == 0)
	{
		USART_SendData(&USART1Handle, &cmd[5], strlen((char *)&cmd[5]));
		USART_SendData(&USART1Handle, (uint8_t *)"\r\n", 2);
	}
	else
	{
		uint8_t response[] = "ERR\r\n";
		USART_SendData(&USART1Handle, response, sizeof(response) - 1);
	}
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
}
