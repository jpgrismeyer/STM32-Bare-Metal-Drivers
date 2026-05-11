#ifndef RCC_DRIVER_H
#define RCC_DRIVER_H

#include <stdint.h>

/*
 * Clock sources (simplificado por ahora)
 */
#define HSI_VALUE     16000000U
#define HSE_VALUE     8000000U

/*
 * API pública
 */
uint32_t RCC_GetPLLOutputClock(void);

uint32_t RCC_GetSYSCLKValue(void);

uint32_t RCC_GetPCLK1Value(void);

uint32_t RCC_GetPCLK2Value(void);

uint32_t RCC_GetHCLKValue(void);

void SystemClock_HSI_Init(void);

#endif
