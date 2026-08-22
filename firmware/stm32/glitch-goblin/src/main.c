#include "stm32f4xx_hal.h"
#include <string.h>

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

UART_HandleTypeDef huart2;

static void LED_Init(void);
static void UART2_Init(void);

int main(void){
    HAL_Init();
    LED_Init();
    UART2_Init();

    const char *message = "Mwahaha Glitch Goblin is now back!\r\n";    // basic uart testing right now

    while(1){
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
        HAL_Delay(500); 
    }
}


static void LED_Init(void){
    __HAL_RCC_GPIOA_CLK_ENABLE();  //turns on the hardware clock for the gpio

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LED_PORT, &gpio);
}
static void UART2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};


    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;    //pa2 and pa3 for tx rx
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200; //comm speed
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}