#include"stm32l47xx.h"


USART_Handle_t USART1Handle;

//Initialize PB6 and PB7 as USART pins.
void USART1_GPIOInits(void)
{
	GPIO_Handle_t USARTPins;

	USARTPins.pGPIOx = GPIOB;
	USARTPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	USARTPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;     //siempre para USART
	USARTPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	USARTPins.GPIO_PinConfig.GPIO_PinAltFunMode = 7;
	USARTPins. GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_HIGH;

	//Usart1 Tx
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&USARTPins);


	//Usart1 Rx
	USARTPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&USARTPins);


}

//Initialize USART1 with specific parameters
void USART1_Inits(void)
{
	USART1Handle.pUSARTx = USART1;
	USART1Handle.USART_Config.USART_Baud = USART_STD_BAUD_9600;
	USART1Handle.USART_Config.USART_Mode = USART_MODE_TXRX;
	USART1Handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
	USART1Handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;

	USART_Init(&USART1Handle);

}


int main(){

#define CMD_BUFFER_SIZE 64

uint8_t rx;
uint8_t cmd_buffer[CMD_BUFFER_SIZE];
uint32_t cmd_index = 0;


   SystemClock_HSI_Init();
   HSI16_ENABLE();

   USART1_CCLK_HSI16();  // USART1 clock source = HSI16

   USART1_GPIOInits();

   USART1_Inits();

uint8_t ready[] = "READY\r\n";
USART_SendData(&USART1Handle, ready, sizeof(ready) - 1);

while (1)
{
	if (USART_ReceiveData(&USART1Handle, &rx, 1U) == USART_OK)
	    {
	        if (rx == '\r' || rx == '\n')
	        {
	            cmd_buffer[cmd_index] = '\0';

	            ProcessCommand(cmd_buffer);

	            cmd_index = 0;
	        }
	        else
	        {
	            if (cmd_index < CMD_BUFFER_SIZE - 1U)
	            {
	                cmd_buffer[cmd_index++] = rx;
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
