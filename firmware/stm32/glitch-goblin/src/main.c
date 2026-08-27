#include "stm32f4xx_hal.h"
#include "board.h"
#include "serum_app.h"
#include "timebase.h"

int main(void)
{
    HAL_Init();

    Board_Init();
    Timebase_Init();
    SerumApp_Init();

    while (1)
    {
        SerumApp_Process();
    }
}