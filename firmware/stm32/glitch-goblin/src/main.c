#include "stm32f4xx_hal.h"
#include <string.h>

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

UART_HandleTypeDef huart2;

volatile uint8_t rx_byte;
volatile uint8_t rx_ready = 0;

static void LED_Init(void);
static void UART2_Init(void);

int main(void)
{
    HAL_Init();

    LED_Init();
    UART2_Init();


    const char *welcome =
        "\r\nGlitch Goblin v0.1\r\n"
        "Commands:\r\n"
        "1 = Glitch Goblin Led on\r\n"
        "0 = Glitch Goblin Led off\r\n"
        "t = Glitch Goblin Led toggle\r\n";
        

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)welcome,
        strlen(welcome),
        HAL_MAX_DELAY
    );

    HAL_UART_Receive_IT(
    &huart2,
    (uint8_t *)&rx_byte,
    1
);

    while (1)
{
    if (rx_ready)
    {
        rx_ready = 0;

        if (rx_byte == '1')
        {
            HAL_GPIO_WritePin(
                LED_PORT,
                LED_PIN,
                GPIO_PIN_SET
            );

            const char *msg =
                "Glitch Goblin Led is now on!\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        else if (rx_byte == '0')
        {
            HAL_GPIO_WritePin(
                LED_PORT,
                LED_PIN,
                GPIO_PIN_RESET
            );

            const char *msg =
                "Glitch Goblin Led is now off!\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        else if (rx_byte == 't')
        {
            HAL_GPIO_TogglePin(
                LED_PORT,
                LED_PIN
            );

            const char *msg =
                "Glitch Goblin Led is now toggled!\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        else if (rx_byte == '?')
        {
            const char *msg =
                "1 = Glitch Goblin Led on\r\n"
                "0 = Glitch Goblin Led off\r\n"
                "t = Glitch Goblin Led toggle\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        HAL_UART_Receive_IT(
            &huart2,
            (uint8_t *)&rx_byte,
            1
        );
    }
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

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);   //interrupt controller
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
void SysTick_Handler(void)
{
    HAL_IncTick();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        rx_ready = 1;
    }
}