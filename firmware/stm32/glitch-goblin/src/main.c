#include "stm32f4xx.h"
#include "drivers/clock.h"
#include "board.h"
#include "serum_app.h"
#include "drivers/timebase.h"

int main(void)
{
    Clock_Init84MHz();

    NVIC_SetPriorityGrouping(0);

    Board_Init();
    Timebase_Init();
    SerumApp_Init();

    while (1)
    {
        SerumApp_Process();
    }
}