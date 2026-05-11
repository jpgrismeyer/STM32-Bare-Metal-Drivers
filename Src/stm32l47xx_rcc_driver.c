#include "stm32l47xx_rcc_driver.h"
#include "stm32l47xx.h"

/*
 * NOTA:
 * Esta versión es "bare minimum funcional".
 * Asume configuración simple (HSI o SYSCLK directo).
 * Luego se extiende con prescalers reales.
 */

/*
 * SYSCLK
 */
uint32_t RCC_GetSYSCLKValue(void)
{
    uint32_t sysclk;

    uint32_t cfgr = RCC->CFGR & (3 << 2); // SWS bits

    switch(cfgr)
    {
        case 0x00: // MSI
            sysclk = 4000000; // simplificado
            break;

        case 0x04: // HSI
            sysclk = HSI_VALUE;
            break;

        case 0x08: // HSE
            sysclk = HSE_VALUE;
            break;

        case 0x0C: // PLL
            sysclk = RCC_GetPLLOutputClock();
            break;

        default:
            sysclk = HSI_VALUE;
    }

    return sysclk;
}

/*
 * PLL (versión mínima)
 * (luego se expande con M/N/R real)
 */
uint32_t RCC_GetPLLOutputClock(void)
{
    // simplificado por ahora
    return HSI_VALUE;
}

/*
 * AHB clock (HCLK)
 */
uint32_t RCC_GetHCLKValue(void)
{
    uint32_t sysclk = RCC_GetSYSCLKValue();

    uint32_t hpre = (RCC->CFGR >> 4) & 0xF;

    // simplificación (sin tabla completa todavía)
    if(hpre < 8)
        return sysclk;

    return sysclk / 2;
}

/*
 * APB1 (PCLK1)
 */
uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t hclk = RCC_GetHCLKValue();

    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;

    if(ppre1 < 4)
        return hclk;

    return hclk / 2;
}

/*
 * APB2 (PCLK2) → USART1 usa esto
 */
uint32_t RCC_GetPCLK2Value(void)
{
    uint32_t hclk = RCC_GetHCLKValue();

    uint32_t ppre2 = (RCC->CFGR >> 11) & 0x7;

    if(ppre2 < 4)
        return hclk;

    return hclk / 2;
}

void SystemClock_HSI_Init(void)
{
    // 1. Encender HSI
    RCC->CR |= (1 << 8);
    while(!(RCC->CR & (1 << 10)));

    // 2. Seleccionar HSI como SYSCLK
    RCC->CFGR &= ~(3 << 0);
    RCC->CFGR |=  (1 << 0);

    // 3. Esperar cambio efectivo
    while(((RCC->CFGR >> 2) & 3) != 1);
}
