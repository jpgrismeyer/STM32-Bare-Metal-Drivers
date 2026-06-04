/*
 * stm32l47xx_adc_driver.c
 *
 *  Created on: May 25, 2025
 *      Author: Juan Pablo Grismeyer
 */


#include "stm32l47xx_adc_driver.h"
#include <stddef.h>

#define ADC_TIMEOUT_COUNT   100000U

void ADC_PeriClockControl(ADC_RegDef_t *pADCx, uint8_t EnorDi)
{
	if (pADCx == ADC1)
	{
		if (EnorDi == ENABLE)
			RCC->AHB2ENR |= (1 << 13); // ADCEN
		else
			RCC->AHB2ENR &= ~(1 << 13);
	}
}

uint8_t ADC_Init(ADC_Handle_t *pADCHandle)
{
	uint32_t timeout;

	ADC_PeriClockControl(pADCHandle->pADCx, ENABLE);

	/*
	 * Enable the ADC internal voltage regulator.
	 * DEEPPWD must be cleared before enabling ADVREGEN.
	 * A short stabilization delay is required after enabling the regulator
	 * before starting calibration (at least 20 us per RM0351).
	 */
	pADCHandle->pADCx->CR &= ~(1U << 29); // clear DEEPPWD
	pADCHandle->pADCx->CR &= ~(1U << 0);  // ensure ADEN = 0
	pADCHandle->pADCx->CR |=  (1U << 28); // ADVREGEN = 1
	for (volatile uint32_t i = 0; i < 1000U; i++); // ~20 us @ 80 MHz

	/*
	 * Run ADC self-calibration (single-ended mode).
	 * ADCAL is cleared by hardware when calibration completes.
	 */
	pADCHandle->pADCx->CR |= (1U << 31); // ADCAL = 1
	timeout = ADC_TIMEOUT_COUNT;
	while (pADCHandle->pADCx->CR & (1U << 31))
	{
		if (timeout-- == 0U) { return ADC_TIMEOUT; }
	}

	/*
	 * Enable the ADC.
	 * Clear ADRDY first (write 1 to clear), then set ADEN.
	 * Wait for ADRDY to confirm the ADC is ready to convert.
	 */
	pADCHandle->pADCx->ISR |= (1U << 0); // clear ADRDY
	pADCHandle->pADCx->CR  |= (1U << 0); // ADEN = 1
	timeout = ADC_TIMEOUT_COUNT;
	while (!(pADCHandle->pADCx->ISR & (1U << 0)))
	{
		if (timeout-- == 0U) { return ADC_TIMEOUT; }
	}

	/* Configure single or continuous conversion mode. */
	if (pADCHandle->ADC_Config.ADC_ContinuousMode)
		pADCHandle->pADCx->CFGR |=  (1U << 13); // CONT = 1
	else
		pADCHandle->pADCx->CFGR &= ~(1U << 13); // CONT = 0 (single)

	/*
	 * Resolution defaults to 12 bits (CFGR RES[1:0] = 00).
	 * No explicit configuration needed unless overriding the default.
	 */

	return ADC_OK;
}

uint8_t ADC_ReadChannel(ADC_RegDef_t *pADCx, uint8_t channel, uint16_t *pResult)
{
	uint32_t timeout;

	if (pResult == NULL) { return ADC_ERROR; }

	/*
	 * Configure the regular sequence for a single conversion:
	 * - SQR1[3:0]  L[3:0] = 0 → 1 conversion in the sequence
	 * - SQR1[10:6] SQ1[4:0]  → channel to convert
	 */
	pADCx->SQR1 &= ~(0x0FU << 0);       // L[3:0] = 0 (1 conversion)
	pADCx->SQR1 &= ~(0x1FU << 6);       // clear SQ1
	pADCx->SQR1 |=  (channel << 6);     // SQ1 = channel

	/*
	 * Wait until any ongoing conversion is complete before starting a new one.
	 * ADSTART is cleared by hardware when the conversion finishes.
	 */
	timeout = ADC_TIMEOUT_COUNT;
	while (pADCx->CR & (1U << 2))
	{
		if (timeout-- == 0U) { return ADC_TIMEOUT; }
	}

	/* Start conversion. */
	pADCx->CR |= (1U << 2); // ADSTART = 1

	/* Wait for end of conversion (EOC flag). */
	timeout = ADC_TIMEOUT_COUNT;
	while (!(pADCx->ISR & (1U << 2)))
	{
		if (timeout-- == 0U) { return ADC_TIMEOUT; }
	}

	/* Reading DR clears the EOC flag. */
	*pResult = (uint16_t)(pADCx->DR & 0x0FFFU);

	return ADC_OK;
}
