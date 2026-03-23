/*
 * stm32l47xx_adc_driver.c
 *
 *  Created on: May 25, 2025
 *      Author: admin
 */


#include "stm32l47xx_adc_driver.h"

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

void ADC_Init(ADC_Handle_t *pADCHandle)
{
	ADC_PeriClockControl(pADCHandle->pADCx, ENABLE);

	// Habilitar el ADC voltage regulator
	pADCHandle->pADCx->CR &= ~(1 << 0); // ADC disabled
	pADCHandle->pADCx->CR |= (1 << 28); // ADVREGEN enabled
	for (volatile int i = 0; i < 1000; i++); // small delay

	// Calibración del ADC
	pADCHandle->pADCx->CR |= (1 << 31); // ADCAL
	while (pADCHandle->pADCx->CR & (1 << 31)); // esperar que termine

	// Habilitar ADC
	pADCHandle->pADCx->ISR |= (1 << 0);  // ADRDY clear
	pADCHandle->pADCx->CR |= (1 << 0);   // ADEN
	while (!(pADCHandle->pADCx->ISR & (1 << 0))); // esperar ADRDY

	// Configurar modo single o continuo
	if (pADCHandle->ADC_Config.ADC_ContinuousMode)
		pADCHandle->pADCx->CFGR |= (1 << 13); // CONT
	else
		pADCHandle->pADCx->CFGR &= ~(1 << 13); // single

	// Resolución (opcional, por defecto 12 bits)
	// pADCHandle->pADCx->CFGR &= ~(3 << 3); // 12-bit (default)
}

uint16_t ADC_ReadChannel(ADC_RegDef_t *pADCx, uint8_t channel)
{
	// Seleccionar canal
	pADCx->SQR1 &= ~(0x1F << 6);        // Limpiar L[3:0] (1 conversion)
	pADCx->SQR1 &= ~(0x1F << 6);        // SQ1 clean
	pADCx->SQR1 |= (channel << 6);      // SQ1 = channel

	// Esperar que no esté en conversión
	while (pADCx->CR & (1 << 2));       // ADSTART

	// Iniciar conversión
	pADCx->CR |= (1 << 2);              // ADSTART

	// Esperar fin de conversión
	while (!(pADCx->ISR & (1 << 2)));   // EOC

	// Leer valor
	uint16_t result = pADCx->DR;

	return result;
}
