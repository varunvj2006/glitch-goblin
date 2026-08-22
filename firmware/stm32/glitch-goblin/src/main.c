#include "stm32f4xx_hal.h"

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

static void LED_Init(void);

int main(void)
{
    HAL_Init();

    LED_Init();

    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(500);
    }
}

static void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LED_PORT, &gpio);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}