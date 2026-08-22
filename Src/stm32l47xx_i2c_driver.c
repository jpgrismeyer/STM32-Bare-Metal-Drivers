#include "stm32l47xx.h"
#include "stm32l47xx_i2c_driver.h"
#include <stddef.h>


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
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if (EnorDi == ENABLE)
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ISER0 |= (1 << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
		}
	}
	else
	{
		if (IRQNumber <= 31)
		{
			*NVIC_ICER0 |= (1 << IRQNumber);
		}
		else if (IRQNumber > 31 && IRQNumber < 64)
		{
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		}
		else if (IRQNumber >= 64 && IRQNumber < 96)
		{
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
		}
	}
}

void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

/*
 * Kicks off a non-blocking master write: configures CR2 (address, NBYTES, write
 * direction, START) exactly like the blocking I2C_MasterSendData, but never busy-waits.
 * AUTOEND is always left disabled here -- completion is driven entirely off TC/STOPF in
 * I2C_EV_IRQHandling, the same software-managed-end logic the blocking function already
 * uses in its non-autoend branch. That keeps one completion path instead of two.
 */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	if ((pI2CHandle == NULL) || (pBuffer == NULL) || (Len == 0U))
	{
		return I2C_ERROR_TIMEOUT;
	}

	if (pI2CHandle->TxRxState != I2C_READY)
	{
		return I2C_ERROR_BUSY;
	}

	pI2CHandle->pTxBuffer = pBuffer;
	pI2CHandle->TxLen = Len;
	pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
	pI2CHandle->DevAddr = SlaveAddr;
	pI2CHandle->Sr = Sr;

	uint32_t tempreg = pI2CHandle->pI2Cx->CR2;
	tempreg &= ~((0x3FFU << I2C_CR2_SADD) | (0xFFU << I2C_CR2_NBYTES) |
	             (1U << I2C_CR2_RD_WRN) | (1U << I2C_CR2_START) |
	             (1U << I2C_CR2_STOP) | (1U << I2C_CR2_AUTOEND));
	tempreg |= ((SlaveAddr & 0x7FU) << 1);
	tempreg |= (Len << I2C_CR2_NBYTES);
	tempreg |= (1U << I2C_CR2_START);
	pI2CHandle->pI2Cx->CR2 = tempreg;

	pI2CHandle->pI2Cx->CR1 |= (1U << I2C_CR1_TXIE) | (1U << I2C_CR1_NACKIE) |
	                          (1U << I2C_CR1_STOPIE) | (1U << I2C_CR1_TCIE) |
	                          (1U << I2C_CR1_ERRIE);

	return I2C_ERROR_NONE;
}

/* Same idea as the send side, but RD_WRN=1 (read) and pulling from RXDR instead of TXDR. */
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	if ((pI2CHandle == NULL) || (pBuffer == NULL) || (Len == 0U))
	{
		return I2C_ERROR_TIMEOUT;
	}

	if (pI2CHandle->TxRxState != I2C_READY)
	{
		return I2C_ERROR_BUSY;
	}

	pI2CHandle->pRxBuffer = pBuffer;
	pI2CHandle->RxLen = Len;
	pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
	pI2CHandle->DevAddr = SlaveAddr;
	pI2CHandle->Sr = Sr;

	uint32_t tempreg = pI2CHandle->pI2Cx->CR2;
	tempreg &= ~((0x3FFU << I2C_CR2_SADD) | (0xFFU << I2C_CR2_NBYTES) |
	             (1U << I2C_CR2_RD_WRN) | (1U << I2C_CR2_START) |
	             (1U << I2C_CR2_STOP) | (1U << I2C_CR2_AUTOEND));
	tempreg |= ((SlaveAddr & 0x7FU) << 1);
	tempreg |= (Len << I2C_CR2_NBYTES);
	tempreg |= (1U << I2C_CR2_RD_WRN);
	tempreg |= (1U << I2C_CR2_START);
	pI2CHandle->pI2Cx->CR2 = tempreg;

	pI2CHandle->pI2Cx->CR1 |= (1U << I2C_CR1_RXIE) | (1U << I2C_CR1_NACKIE) |
	                          (1U << I2C_CR1_STOPIE) | (1U << I2C_CR1_TCIE) |
	                          (1U << I2C_CR1_ERRIE);

	return I2C_ERROR_NONE;
}

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_TXIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_NACKIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_STOPIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_TCIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_ERRIE);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pTxBuffer = NULL;
	pI2CHandle->TxLen = 0U;
}

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_RXIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_NACKIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_STOPIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_TCIE);
	pI2CHandle->pI2Cx->CR1 &= ~(1U << I2C_CR1_ERRIE);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pRxBuffer = NULL;
	pI2CHandle->RxLen = 0U;
}

/*
 * Event IRQ: TXIS, RXNE, NACKF, TC, STOPF. All the "this is a normal step in an active
 * transfer" flags live here -- NACKF included, since on STM32's I2C peripheral a NACK is
 * part of the regular transfer sequence (the ST HAL's EV handler treats it the same way),
 * not a bus fault. It's checked again, defensively, in I2C_ER_IRQHandling below.
 */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	uint32_t isr = pI2CHandle->pI2Cx->ISR;
	uint32_t cr1 = pI2CHandle->pI2Cx->CR1;

	if ((isr & (1U << I2C_ISR_TXIS)) && (cr1 & (1U << I2C_CR1_TXIE)))
	{
		if ((pI2CHandle->TxRxState == I2C_BUSY_IN_TX) && (pI2CHandle->TxLen > 0U))
		{
			pI2CHandle->pI2Cx->TXDR = *(pI2CHandle->pTxBuffer);
			pI2CHandle->pTxBuffer++;
			pI2CHandle->TxLen--;
		}
	}

	if ((isr & (1U << I2C_ISR_RXNE)) && (cr1 & (1U << I2C_CR1_RXIE)))
	{
		if ((pI2CHandle->TxRxState == I2C_BUSY_IN_RX) && (pI2CHandle->RxLen > 0U))
		{
			*(pI2CHandle->pRxBuffer) = (uint8_t)pI2CHandle->pI2Cx->RXDR;
			pI2CHandle->pRxBuffer++;
			pI2CHandle->RxLen--;
		}
	}

	if ((isr & (1U << I2C_ISR_NACKF)) && (cr1 & (1U << I2C_CR1_NACKIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_NACKCF);

		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}

		I2C_ApplicationEventCallBack(pI2CHandle, I2C_ERROR_EVENT_NACK);
	}

	/* TC fires once NBYTES have all been shifted (AUTOEND=0, no reload). Either issue
	 * STOP ourselves (no repeated START requested), or -- if Sr requested one -- leave
	 * the bus held and hand control back to the app so it can chain the next transfer. */
	if ((isr & (1U << I2C_ISR_TC)) && (cr1 & (1U << I2C_CR1_TCIE)))
	{
		if (pI2CHandle->Sr == I2C_SR_DISABLE)
		{
			pI2CHandle->pI2Cx->CR2 |= (1U << I2C_CR2_STOP);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
			I2C_ApplicationEventCallBack(pI2CHandle, I2C_EVENT_TX_CMPLT);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
			I2C_ApplicationEventCallBack(pI2CHandle, I2C_EVENT_RX_CMPLT);
		}
	}

	/* STOPF: STOP was actually seen on the bus -- the transfer we forced STOP on above
	 * (Sr == I2C_SR_DISABLE case) is now really finished. */
	if ((isr & (1U << I2C_ISR_STOPF)) && (cr1 & (1U << I2C_CR1_STOPIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_STOPCF);

		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
			I2C_ApplicationEventCallBack(pI2CHandle, I2C_EVENT_TX_CMPLT);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
			I2C_ApplicationEventCallBack(pI2CHandle, I2C_EVENT_RX_CMPLT);
		}
	}
}

/*
 * Error IRQ: BERR, ARLO, OVR -- genuine bus faults, never part of a normal transfer.
 * NACKF is re-checked here too (see the comment in I2C_EV_IRQHandling): if it already
 * fired there this is simply a no-op, since the flag is already clear.
 */
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	uint32_t isr = pI2CHandle->pI2Cx->ISR;
	uint32_t cr1 = pI2CHandle->pI2Cx->CR1;
	uint8_t had_error = 0U;

	if ((isr & (1U << I2C_ISR_NACKF)) && (cr1 & (1U << I2C_CR1_NACKIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_NACKCF);
		had_error = 1U;
		I2C_ApplicationEventCallBack(pI2CHandle, I2C_ERROR_EVENT_NACK);
	}

	if ((isr & (1U << I2C_ISR_BERR)) && (cr1 & (1U << I2C_CR1_ERRIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_BERRCF);
		had_error = 1U;
		I2C_ApplicationEventCallBack(pI2CHandle, I2C_ERROR_EVENT_BERR);
	}

	if ((isr & (1U << I2C_ISR_ARLO)) && (cr1 & (1U << I2C_CR1_ERRIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_ARLOCF);
		had_error = 1U;
		I2C_ApplicationEventCallBack(pI2CHandle, I2C_ERROR_EVENT_ARLO);
	}

	if ((isr & (1U << I2C_ISR_OVR)) && (cr1 & (1U << I2C_CR1_ERRIE)))
	{
		pI2CHandle->pI2Cx->ICR |= (1U << I2C_ICR_OVRCF);
		had_error = 1U;
		I2C_ApplicationEventCallBack(pI2CHandle, I2C_ERROR_EVENT_OVR);
	}

	if (had_error)
	{
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_CloseSendData(pI2CHandle);
		}
		else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_CloseReceiveData(pI2CHandle);
		}
	}
}

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

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint8_t FlagName)
{
	if (pI2Cx->ISR & (1U << FlagName))
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

__attribute__((weak)) void I2C_ApplicationEventCallBack(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
	/* Weak default: applications override this to react to IT transfer events.
	 * Left empty here so the driver still links even before an app defines its own. */
	(void)pI2CHandle;
	(void)AppEv;
}

