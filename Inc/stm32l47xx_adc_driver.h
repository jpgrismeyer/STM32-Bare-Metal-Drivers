/*
 * stm32l47xx_adc_driver.h
 *
 *  Created on: May 25, 2025
 *      Author: Juan Pablo Grismeyer
 */

#ifndef INC_STM32L47XX_ADC_DRIVER_H_
#define INC_STM32L47XX_ADC_DRIVER_H_

#include "stm32l47xx.h"

/*
 * ADC return codes
 */
#define ADC_OK       0
#define ADC_TIMEOUT  1
#define ADC_ERROR    2

/*
 * ADC Configuration structure
 */
typedef struct
{
	uint8_t ADC_Resolution;     /* 12-bit by default (CFGR RES = 00) */
	uint8_t ADC_ContinuousMode; /* 1 = continuous, 0 = single conversion */
	uint8_t ADC_Channel;        /* channel to read */
} ADC_Config_t;

/*
 * ADC Handle structure
 */
typedef struct
{
	ADC_RegDef_t *pADCx;
	ADC_Config_t  ADC_Config;
} ADC_Handle_t;


/*
 * Peripheral clock control
 */
void    ADC_PeriClockControl(ADC_RegDef_t *pADCx, uint8_t EnorDi);

/*
 * Init: enables voltage regulator, runs self-calibration, enables ADC.
 * Returns ADC_OK on success, ADC_TIMEOUT if any step exceeds the timeout.
 */
uint8_t ADC_Init(ADC_Handle_t *pADCHandle);

/*
 * Reads a single sample from the given channel.
 * Stores the 12-bit result in *pResult.
 * Returns ADC_OK, ADC_TIMEOUT, or ADC_ERROR.
 */
uint8_t ADC_ReadChannel(ADC_RegDef_t *pADCx, uint8_t channel, uint16_t *pResult);


#endif /* INC_STM32L47XX_ADC_DRIVER_H_ */
