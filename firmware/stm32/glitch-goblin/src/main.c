#include "stm32f4xx.h"
#include "board.h"
#include "serum_app.h"
#include "timebase.h"

int main(void)
{
    NVIC_SetPriorityGrouping(0);

    Board_Init();
    Timebase_Init();
    SerumApp_Init();

    while (1)
    {
        SerumApp_Process();
    }
}