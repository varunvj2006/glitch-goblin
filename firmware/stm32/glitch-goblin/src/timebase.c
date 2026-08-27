#include "timebase.h"
#include "stm32f4xx.h"

static volatile uint32_t ms_ticks = 0;

void Timebase_Init(void)
{
    SysTick->LOAD =
        (SystemCoreClock / 1000U) - 1U;    //1ms tick so 16Mhz / 1000 = 16000 - 1 = 15999

    SysTick->VAL = 0;   //reset counter

    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |   //processor clk
        SysTick_CTRL_TICKINT_Msk |   //generate interrupt when it reaches 0
        SysTick_CTRL_ENABLE_Msk;  //start systick
}

uint32_t Timebase_Millis(void)
{
    return ms_ticks;
}

void Timebase_DelayMs(
    uint32_t delay_ms
)
{
    uint32_t start =
        Timebase_Millis();

    while (
        (Timebase_Millis() - start)
        < delay_ms
    )
    {
    }
}

void SysTick_Handler(void)
{
    ms_ticks++;
}