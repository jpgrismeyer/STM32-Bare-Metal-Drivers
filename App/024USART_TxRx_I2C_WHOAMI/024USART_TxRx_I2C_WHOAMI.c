#include"stm32l47xx.h"

#define LPS22HB_ADDR      0x5D
#define WHO_AM_I_REG      0x0F
#define EXPECTED_VALUE    0xB1
#define MY_ADDR           0x61

USART_Handle_t USART1Handle;
I2C_Handle_t I2C2Handle;

void ProcessCommand(uint8_t *cmd);

//Initialize PB10 and PB11 as I2C pins.
void I2C2_GPIOInits(void)
{
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;
	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
	I2CPins.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD;
	I2CPins.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
	I2CPins.GPIO_PinConfig.GPIO_PinAltFunMode = 4;
	I2CPins. GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;

	//I2C2 SCL
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
	GPIO_Init(&I2CPins);

	//I2C2 SDA
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
	GPIO_Init(&I2CPins);
}

//Initialize I2C2 with specific parameters.
void I2C2_Inits(void)
{
	I2C2Handle.pI2Cx = I2C2;
	I2C2Handle.I2CConfig.I2C_DeviceAddress = MY_ADDR;
	I2C2Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C2Handle.I2CConfig.I2C_NoStretch = I2C_NOSTRETCH_DISABLE;
	I2C2Handle.I2CConfig.I2C_AutoEnd = I2C_AUTOEND_DISABLE;

	I2C_Init(&I2C2Handle);
}

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

//initialize GPIO as User Button (PC13)
void GPIO_ButtonInit(void)
{
	GPIO_Handle_t GPIOBtn;

	//this is btn gpio configuration
	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOBtn);

}

//initialize GPIO as User LED2 (PB14)
void GPIO_LEDInit(void)
{
	GPIO_Handle_t GPIOLED;

	//this is btn gpio configuration
	GPIOLED.pGPIOx = GPIOB;
	GPIOLED.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	GPIOLED.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT;
	GPIOLED.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
	GPIOLED.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
	GPIOLED.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_NO_PUPD;

	GPIO_Init(&GPIOLED);

}

int main(){

#define CMD_BUFFER_SIZE 64

uint8_t rx;
uint8_t cmd_buffer[CMD_BUFFER_SIZE];
uint32_t cmd_index = 0;


   SystemClock_HSI_Init();
   HSI16_ENABLE();

   USART1_CCLK_HSI16();  // USART1 clock source = HSI16
   I2C2_CCLK_HSI16();    // I2C2 clock source = HSI16

   USART1_GPIOInits();
   I2C2_GPIOInits();

   USART1_Inits();
   I2C2_Inits();

   GPIO_ButtonInit();
   GPIO_LEDInit();

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
    else if (strcmp((char *)cmd, "LED ON") == 0)
    {
        GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, ENABLE);

        uint8_t response[] = "OK\r\n";
        USART_SendData(&USART1Handle, response, sizeof(response) - 1);
    }
    else if (strcmp((char *)cmd, "LED OFF") == 0)
    {
        GPIO_WriteToOutputPin(GPIOB, GPIO_PIN_NO_14, DISABLE);

        uint8_t response[] = "OK\r\n";
        USART_SendData(&USART1Handle, response, sizeof(response) - 1);
    }
    else if (strcmp((char *)cmd, "APP_ID?") == 0)
    {
        uint8_t response[] = "USART_CMD_CONSOLE_POLLING\r\n";
        USART_SendData(&USART1Handle, response, sizeof(response) - 1);
    }
    else if (strcmp((char *)cmd, "BUTTON?") == 0)
    {
        if (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13) == 0)
        {
            uint8_t response[] = "PRESSED\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
        else
        {
            uint8_t response[] = "RELEASED\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
    }
    else if (strcmp((char *)cmd, "I2C WHOAMI") == 0)
    {
        uint8_t reg_addr = WHO_AM_I_REG;
        uint8_t who_am_i_value = 0;
        uint8_t status;

        status = I2C_MasterSendData(&I2C2Handle, &reg_addr, 1, LPS22HB_ADDR, I2C_SR_ENABLE);

        if (status == I2C_ERROR_NONE)
        {
            status = I2C_MasterReceiveData(&I2C2Handle, &who_am_i_value, 1, LPS22HB_ADDR, I2C_SR_DISABLE);
        }

        if ((status == I2C_ERROR_NONE) && (who_am_i_value == EXPECTED_VALUE))
        {
            uint8_t response[] = "0xB1\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
        else if (status == I2C_ERROR_NACK)
        {
            uint8_t response[] = "I2C NACK\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
        else if (status == I2C_ERROR_TIMEOUT)
        {
            uint8_t response[] = "I2C TIMEOUT\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
        else
        {
            uint8_t response[] = "WHOAMI FAIL\r\n";
            USART_SendData(&USART1Handle, response, sizeof(response) - 1);
        }
    }else
    {
        uint8_t response[] = "ERR\r\n";
        USART_SendData(&USART1Handle, response, sizeof(response) - 1);
    }
}
