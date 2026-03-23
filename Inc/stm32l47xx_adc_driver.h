/*
 * stm32l47xx_adc_driver.h
 *
 *  Created on: May 25, 2025
 *      Author: admin
 */

#ifndef INC_STM32L47XX_ADC_DRIVER_H_
#define INC_STM32L47XX_ADC_DRIVER_H_

#include "stm32l47xx.h"

/*
 * ADC Configuration structure
 */
typedef struct
{
	uint8_t ADC_Resolution;     // 12 bits por defecto
	uint8_t ADC_ContinuousMode; // 1 = continuous, 0 = single
	uint8_t ADC_Channel;        // canal a leer
} ADC_Config_t;

/*
 * ADC Handle structure
 */
typedef struct
{
	ADC_RegDef_t *pADCx;
	ADC_Config_t ADC_Config;
} ADC_Handle_t;


/*
 * Function prototypes
 */
void ADC_PeriClockControl(ADC_RegDef_t *pADCx, uint8_t EnorDi);
void ADC_Init(ADC_Handle_t *pADCHandle);
uint16_t ADC_ReadChannel(ADC_RegDef_t *pADCx, uint8_t channel);



#endif /* INC_STM32L47XX_ADC_DRIVER_H_ */
