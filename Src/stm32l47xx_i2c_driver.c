#include "stm32l47xx.h"
#include "stm32l47xx_i2c_driver.h"


void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi){

	if(EnorDi==ENABLE)
		{
			if(pI2Cx==I2C1)
			{
				I2C1_PCLK_EN();
			}else if (pI2Cx==I2C2)
			{
				I2C2_PCLK_EN();
			}else if (pI2Cx==I2C3)
			{
				I2C3_PCLK_EN();
			}
		}
			else if(EnorDi==DISABLE)
				{
					if(pI2Cx==I2C1)
					{
						I2C1_PCLK_DI();
					}else if (pI2Cx==I2C2)
					{
						I2C2_PCLK_DI();
					}else if (pI2Cx==I2C3)
					{
						I2C3_PCLK_DI();
					}
				}
}

void I2C_Init(I2C_Handle_t *pI2CHandle){

	uint32_t tempreg = 0;

	    // 1. Habilitar reloj del periférico
	    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

	    // 2. Configuración de CR1 (Read-Modify-Write)
	    I2C_PeripheralControl(pI2CHandle->pI2Cx, DISABLE); // Debe estar OFF para configurar

	    tempreg = pI2CHandle->pI2Cx->CR1;
	    if (pI2CHandle->I2CConfig.I2C_NoStretch == I2C_NOSTRETCH_ENABLE) {
	        tempreg |= (1 << I2C_CR1_NOSTRETCH);
	    } else {
	        tempreg &= ~(1 << I2C_CR1_NOSTRETCH);
	    }
	    pI2CHandle->pI2Cx->CR1 = tempreg;

	    // 3. Configuración de TIMINGR (Sobreescritura directa, es un valor calculado)
	    if (pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_5KHZ)
	    {
	        // Velocidad ultra lenta para pruebas o cables largos
	        pI2CHandle->pI2Cx->TIMINGR = I2C_TIMING_5KHZ_16MHZ;
	    }
	    else if (pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_SM) {
	        pI2CHandle->pI2Cx->TIMINGR = I2C_TIMING_100KHZ_16MHZ;
	    } else {
	        pI2CHandle->pI2Cx->TIMINGR = I2C_TIMING_400KHZ_16MHZ;
	    }

	    // 4. Configuración de OAR1 (Dirección propia)
	    tempreg = pI2CHandle->pI2Cx->OAR1;
	    tempreg &= ~((0x7F << 1) | (1 << I2C_OAR1_OA1EN)); // Limpiar antes
	    tempreg |= (pI2CHandle->I2CConfig.I2C_DeviceAddress << 1);
	    tempreg |= (1 << I2C_OAR1_OA1EN);
	    pI2CHandle->pI2Cx->OAR1 = tempreg;

	    // 5. Configuración de CR2 (Default Autoend)
	    tempreg = pI2CHandle->pI2Cx->CR2;
	    if(pI2CHandle->I2CConfig.I2C_AutoEnd == ENABLE) {
	        tempreg |= (1 << I2C_CR2_AUTOEND);
	    } else {
	        tempreg &= ~(1 << I2C_CR2_AUTOEND);
	    }
	    pI2CHandle->pI2Cx->CR2 = tempreg;

	    // 6. Habilitar periférico
	    I2C_PeripheralControl(pI2CHandle->pI2Cx, ENABLE);

}

void I2C_DeInit(I2C_RegDef_t *pI2Cx){


			if(pI2Cx==I2C1)
			{
				I2C1_REG_RESET();
			}else if (pI2Cx==I2C2)
			{
				I2C2_REG_RESET();
			}else if (pI2Cx==I2C3)
			{
				I2C3_REG_RESET();
			}
}


//int I2C_MemRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint8_t Reg, uint8_t *pBuf, uint32_t Len);



uint8_t I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr){

	uint32_t tempreg = pI2CHandle->pI2Cx->CR2;
	uint32_t timeout_counter = 0;
	uint32_t timeout_max = 1000000;

	    // --- PASO 1: Limpieza del registro de control ---
	    tempreg &= ~((0x3FF << I2C_CR2_SADD) |
	                 (0xFF  << I2C_CR2_NBYTES) |
	                 (1     << I2C_CR2_RD_WRN) |
	                 (1     << I2C_CR2_START) |
	                 (1     << I2C_CR2_STOP) );

	    // --- PASO 2: Configuración de la dirección y longitud ---
	    tempreg |= ((SlaveAddr & 0x7F) << 1);
	    tempreg |= (Len << I2C_CR2_NBYTES);

	    // Modo Escritura (0)
	    tempreg &= ~(1 << I2C_CR2_RD_WRN);

	    // CONDICIONAL DE AUTOEND: Si el usuario lo configuró en el handle, se setea.
		// Si vamos a usar Repeated Start (Sr=ENABLE), el AUTOEND DEBE estar deshabilitado.
		if (pI2CHandle->I2CConfig.I2C_AutoEnd == I2C_AUTOEND_ENABLE )
		{
			tempreg |= (1 << I2C_CR2_AUTOEND);
		}



	    // --- PASO 3: Iniciar la comunicación ---
	    tempreg |= (1 << I2C_CR2_START);
	    pI2CHandle->pI2Cx->CR2 = tempreg;

	    // --- PASO 4: Loop de envío de datos ---
	    for (uint32_t i = 0; i < Len; i++)
	    {
	        // Esperamos a que el registro de transmisión esté vacío
	        while ( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TXIS)) )
	        {
					if (pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_NACKF))
					{
						pI2CHandle->pI2Cx->ICR |= (1 << I2C_ICR_NACKCF); // Limpiar NACK
						pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP);   // Forzar STOP para liberar bus
						return I2C_ERROR_NACK; // Retorna 1
					}
					if (++timeout_counter >= timeout_max) return I2C_ERROR_TIMEOUT;
				}

	        pI2CHandle->pI2Cx->TXDR = pBuffer[i];
	    }

	    // --- PASO 5: Gestión de cierre ---
	    if (pI2CHandle->I2CConfig.I2C_AutoEnd == ENABLE)
	    {
	        // Esperamos a que el hardware genere el STOP
	        while ( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_STOPF)) );
	        // Limpiamos la bandera para la próxima vez
	        pI2CHandle->pI2Cx->ICR |= (1 << I2C_ICR_STOPCF);
	    }
	    else
	    {
	// Si AUTOEND = 0, esperamos a que se terminen de mandar los bytes (TC)
			while (!(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TC)));

			// Si NO hay Repeated Start, forzamos el STOP manual
			if (Sr == I2C_SR_DISABLE)
			{
				pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP);
				while (!(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_STOPF)));
				pI2CHandle->pI2Cx->ICR |= (1 << I2C_ICR_STOPCF);
			}
			// Si Sr = ENABLE, salimos sin hacer nada. El bus queda ocupado.
	    }


	    return I2C_ERROR_NONE;
}


uint8_t I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	uint32_t timeout = 100000;
	// 1. Asegurar que el registro CR2 esté limpio antes de configurar
	    pI2CHandle->pI2Cx->CR2 &= ~((0x3FF << I2C_CR2_SADD) | (0xFF << I2C_CR2_NBYTES) | (1 << I2C_CR2_RD_WRN) | (1 << I2C_CR2_AUTOEND));

	// 1. CHEQUEO INTELIGENTE DE BUSY
		// Espera si está ocupado (BUSY=1) Y nosotros NO tenemos el control (TC=0).
		// Si TC=1, significa que venimos de un Transmit sin STOP (Repeated Start), entonces pasamos de largo.
		//while ( (pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_BUSY)) && !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TC)) );

	    // 4. Configuración de CR2
	    uint32_t tempreg = 0;
	    tempreg |= ((uint32_t)SlaveAddr << 1);
	    tempreg |= (Len << I2C_CR2_NBYTES);    // NBYTES suele ser la posición (16)
	    tempreg |= (1U << I2C_CR2_RD_WRN);     // 1 = Read

	    if (pI2CHandle->I2CConfig.I2C_AutoEnd == ENABLE) {
	        tempreg |= (1U << I2C_CR2_AUTOEND);
	    }

	    // 5. Iniciar (Configuración + START en un solo paso)
	    tempreg |= (1U << I2C_CR2_START);
	    pI2CHandle->pI2Cx->CR2 = tempreg;


	    // 6. Bucle de recepción
	    for (uint32_t i = 0; i < Len; i++) {
	        timeout = 100000;

	        // Esperar RXNE (bit 2)
	        while (!(pI2CHandle->pI2Cx->ISR & (1U << I2C_ISR_RXNE))) {
	            // Chequeo de NACK (bit 4)
	            if (pI2CHandle->pI2Cx->ISR & (1U << I2C_ISR_NACKF)) {
	                pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_NACKCF);
	                pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_STOP);
	                return I2C_ERROR_NACK;
	            }
	            if (--timeout == 0) return I2C_ERROR_TIMEOUT;
	        }
	        // CREÁ ESTA VARIABLE TEMPORAL SOLO PARA DEBUGEAR
	       // uint8_t valor_leido = (uint8_t)pI2CHandle->pI2Cx->RXDR;

	        pBuffer[i] = (uint8_t)pI2CHandle->pI2Cx->RXDR;
	        asm("nop"); // <--- PONÉ EL BREAKPOINT ACÁ

	    }

	    // 7. Cierre
	    if (pI2CHandle->I2CConfig.I2C_AutoEnd == ENABLE) {
	        timeout = 100000;
	        while (!(pI2CHandle->pI2Cx->ISR & (1U << I2C_ISR_STOPF))) {
	            if (--timeout == 0) return I2C_ERROR_TIMEOUT;
	        }
	        pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_STOPCF);
	    }
	    else
	    	{
	    		// Esperamos a que los bytes terminen de entrar
	    		while (!(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TC)));

	    		// Si terminamos y NO hay Repeated Start pendiente, forzamos el STOP
	    		if (Sr == I2C_SR_DISABLE)
	    		{
	    			pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP);
	    			while (!(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_STOPF)));
	    			pI2CHandle->pI2Cx->ICR |= (1 << I2C_ICR_STOPCF);
	    		}
	    	}

	    return I2C_ERROR_NONE;

}

/*
 *IRQ Configuration and ISR handling
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority);


/*
*	Other Peripheral Control APIs
*/

void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi){

	if (EnOrDi == ENABLE)
		{
			pI2Cx->CR1 |= (1 << I2C_CR1_PE);
		}else
		{
			pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);
		}
}
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi);

void I2C_ApplicationEventCallBack(I2C_Handle_t *pI2CHandle, uint8_t AppEv);

