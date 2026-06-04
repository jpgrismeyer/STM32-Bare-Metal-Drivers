#include "stm32l47xx.h"

USART_Handle_t UART4Handle;

static void delay(void)
{
	for (volatile uint32_t i = 0; i < 400000U; i++);
}

// Initialize PA0 and PA1 as UART4 pins on the Arduino connector.
// Arduino D1 -> PA0 -> UART4_TX
// Arduino D0 -> PA1 -> UART4_RX
void UART4_GPIOInits(void)
{
	GPIO_Handle_t UARTPins;

	UARTPins.pGPIOx = GPIOA;
	UARTPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	UARTPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	UARTPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	UARTPins.GPIO_PinConfig.GPIO_PinAltFunMode = 8;
	UARTPins.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	// UART4 TX on Arduino D1
	UARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GPIO_Init(&UARTPins);

	// UART4 RX on Arduino D0
	UARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_1;
	GPIO_Init(&UARTPins);
}

void UART4_Inits(void)
{
	UART4Handle.pUSARTx = UART4;
	UART4Handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
	UART4Handle.USART_Config.USART_Mode = USART_MODE_TXRX;
	UART4Handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
	UART4Handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
	UART4Handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;
	UART4Handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;

	USART_Init(&UART4Handle);
}

// Initialize PB14 as a visual marker / optional logic analyzer trigger.
void GPIO_LEDInit(void)
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

int main(void)
{
	uint8_t banner[] = "\r\nUART4 LOGIC ANALYZER TEST\r\n";
	uint8_t text[] = "UART4 TX PA0 ARD.D1 9600 8N1\r\n";
	uint8_t pattern_55[] = {0x55, 0x55, 0x55, 0x55, '\r', '\n'};
	uint8_t pattern_a5[] = {0xA5, 0xA5, 0xA5, 0xA5, '\r', '\n'};

	SystemClock_HSI_Init();
	HSI16_ENABLE();
	UART4_CCLK_HSI16();

	GPIO_LEDInit();
	UART4_GPIOInits();
	UART4_Inits();

	USART_SendData(&UART4Handle, banner, sizeof(banner) - 1U);

	while (1)
	{
		GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_14);

		USART_SendData(&UART4Handle, text, sizeof(text) - 1U);
		USART_SendData(&UART4Handle, pattern_55, sizeof(pattern_55));
		USART_SendData(&UART4Handle, pattern_a5, sizeof(pattern_a5));

		delay();
	}
}
