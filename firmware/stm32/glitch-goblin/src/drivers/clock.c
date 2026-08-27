#include "drivers/clock.h"
#include "stm32f4xx.h"

void Clock_Init84MHz(void)
{
    RCC->CR |= RCC_CR_HSION;

    while (!(RCC->CR & RCC_CR_HSIRDY))
    {
    }

    FLASH->ACR =
        FLASH_ACR_PRFTEN |
        FLASH_ACR_ICEN |
        FLASH_ACR_DCEN |
        FLASH_ACR_LATENCY_2WS;

    RCC->CR &= ~RCC_CR_PLLON;

    while (RCC->CR & RCC_CR_PLLRDY)
    {
    }

    RCC->PLLCFGR =
        (16U << 0) |
        (336U << 6) |
        (1U << 16) |
        (7U << 24);

    RCC->CFGR &= ~(
        RCC_CFGR_HPRE |
        RCC_CFGR_PPRE1 |
        RCC_CFGR_PPRE2
    );

    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    RCC->CR |= RCC_CR_PLLON;

    while (!(RCC->CR & RCC_CR_PLLRDY))
    {
    }

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while (
        (RCC->CFGR & RCC_CFGR_SWS) !=
        RCC_CFGR_SWS_PLL
    )
    {
    }

    SystemCoreClockUpdate();
}

uint32_t Clock_GetAPB1Hz(void)
{
    return 42000000U;
}

uint32_t Clock_GetAPB2Hz(void)
{
    return 84000000U;
}