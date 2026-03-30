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


		//Enable Peripheral clock
		I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

		uint32_t tempreg=0;
	//--- Register CR1 ---
	    // Peripheral Disable
	    I2C_PeripheralControl(pI2CHandle->pI2Cx, DISABLE);

	    //NoStretch config
	    if (pI2CHandle->I2CConfig.I2C_NoStretch == I2C_NOSTRETCH_ENABLE) {
	        tempreg |= (1 << I2C_CR1_NOSTRETCH); // Set NOSTRETCH bit
	    } else {
	        tempreg &= ~(1 << I2C_CR1_NOSTRETCH); // Clear NOSTRETCH bit
	    }
	    pI2CHandle->pI2Cx->CR1 = tempreg;

	    //--- TIMINGR Register ---
	    // In L4 series, this register represents the speed
	    // Note: I2C_SCLSpeed must be the 32 bits hexadecimal value for TIMINGR
		if (pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
		{
			// El usuario pidió 100kHz o menos -> Cargamos valor Standard Mode para 16MHz
			pI2CHandle->pI2Cx->TIMINGR = I2C_TIMING_100KHZ_16MHZ;
		}
		else
		{
			// El usuario pidió más (asumimos 400kHz) -> Cargamos Fast Mode para 16MHz
			pI2CHandle->pI2Cx->TIMINGR = I2C_TIMING_400KHZ_16MHZ;
		}

	    //--- OAR1 Register (Own Address) ---
	    tempreg = 0;
	    // Configure address (7 bits) shifted 1 position
	    tempreg |= (pI2CHandle->I2CConfig.I2C_DeviceAddress << 1);
	    // Enable own address (OA1EN)
	    tempreg |= (1 << I2C_OAR1_OA1EN);
	    pI2CHandle->pI2Cx->OAR1 = tempreg;

	    //--- REGISTRO CR2 ---
	    //Disable autoend
	    pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_AUTOEND);

	    //--- Peripheral Enable ---
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


int I2C_MemRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr, uint8_t Reg, uint8_t *pBuf, uint32_t Len);



uint8_t I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr){

	uint32_t timeout_count = 0;
	const uint32_t TIMEOUT_MAX = 100000; // Valor arbitrario para el ejemplo
	uint32_t tempreg = 0;

	    // 1. Prepare the CR2 register configuration in a temporary variable (RAM)
	    // Read the current state of CR2 to avoid overwriting unrelated bits
	    tempreg = pI2CHandle->pI2Cx->CR2;

	    // Clear the fields we are about to configure:
	    // SADD (0:9), NBYTES (16:23), RD_WRN (10), START (13), STOP (14)
	    tempreg &= ~((0x3FF << I2C_CR2_SADD) | (0xFF << I2C_CR2_NBYTES) | (1 << I2C_CR2_RD_WRN) | (1 << I2C_CR2_START) | (1 << I2C_CR2_STOP));

	    // A. Configure the Slave Address (SADD)
	    // Shifted by 1 because bit 0 is reserved for 10-bit addressing mode
	    tempreg |= ((SlaveAddr & 0x7F) << 1);

	    // B. Configure the number of bytes to be transmitted (NBYTES)
	    tempreg |= ((uint32_t)Len << I2C_CR2_NBYTES);

	    // C. Set Transfer Direction to Write (RD_WRN = 0)
	    // (Already cleared in the masking step above)

	    // D. Set the START bit in our temporary variable
	    tempreg |= (1 << I2C_CR2_START);

	    // --- CRITICAL STEP: Atomic Update ---
	    // Write the entire configuration to the actual CR2 register at once.
	    // This triggers the hardware to generate the START condition and send the address.
	    pI2CHandle->pI2Cx->CR2 = tempreg;


	    // 2. Data Transmission Loop
	        for(uint32_t i = 0; i < Len; i++)
	        {
	            // Wait for TXIS (Ready to transmit) OR NACK (Error) OR Timeout
	            timeout_count = 0;
	            while( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TXIS)) )
	            {
	                // Check if Slave sent a NACK (NACKF bit 4)
	                if(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_NACKF)) {
	                    pI2CHandle->pI2Cx->ICR |= (1 << I2C_ICR_NACKCF); // Clear NACK flag
	                    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP); // Generate STOP
	                    return I2C_ERROR_NACK;
	                }

	                // Safety Timeout
	                if(timeout_count++ > TIMEOUT_MAX) return I2C_ERROR_TIMEOUT;
	            }

	            pI2CHandle->pI2Cx->TXDR = pBuffer[i];
	        }

	        // 3. Wait for Transfer Complete (TC)
	        timeout_count = 0;
	        while( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TC)) ) {
	            if(timeout_count++ > TIMEOUT_MAX) return I2C_ERROR_TIMEOUT;
	        }

	        // 4. Generate STOP
	        pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP);

	        return I2C_ERROR_NONE;
}


uint8_t I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pBuffer, uint32_t Len, uint8_t SlaveAddr)
{
    uint32_t tempreg = 0;

    // 1. Prepare CR2 for Reception
    tempreg = pI2CHandle->pI2Cx->CR2;
    tempreg &= ~((0x3FF << I2C_CR2_SADD) | (0xFF << I2C_CR2_NBYTES) | (1 << I2C_CR2_RD_WRN) | (1 << I2C_CR2_START) | (1 << I2C_CR2_STOP));

    tempreg |= ((SlaveAddr & 0x7F) << 1);
    tempreg |= ((uint32_t)Len << I2C_CR2_NBYTES);
    tempreg |= (1 << I2C_CR2_RD_WRN); // RD_WRN = 1 (READ MODE)
    tempreg |= (1 << I2C_CR2_START); // START

    pI2CHandle->pI2Cx->CR2 = tempreg;

    // 2. Receiving Loop
    for(uint32_t i = 0; i < Len; i++)
    {
        // Wait for RXNE (Receive Not Empty) flag in ISR
        while( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_RXNE)) );

        // Read data from RXDR
        pBuffer[i] = pI2CHandle->pI2Cx->RXDR;
    }

    // 3. Wait for Transfer Complete and Generate STOP
    while( !(pI2CHandle->pI2Cx->ISR & (1 << I2C_ISR_TC)) );
    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_STOP);

    return 0;
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

