#include "board.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

static void LED_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);

void Board_Init(void)
{
    LED_Init();
    UART1_Init();
    UART2_Init();
}

void Board_LED_Toggle(void)
{
    GPIOA->ODR ^= (1U << 5);    //a easy XOR for toglling
}


static void LED_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // enable reg-clock using the existing bit-mask for GPIOA
    GPIOA->MODER &= ~(3U << (5 * 2));  // clear the bits for pin 5  (2 BITS)
    GPIOA->MODER |=  (1U << (5 * 2));  // set the 01 for pin 5 for output mode 
    GPIOA->OTYPER &= ~(1U << 5);     //set the type to push/pull (1 BIT)
    GPIOA->PUPDR &= ~(3U << (5 * 2));  //reset the pull-up/pull-down bits to none for pin 5  (2 BITS)
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));  //reset the speed bits to low for pin 5 (2 BITS)
}


void Board_UART1_Write(
    const uint8_t *data,
    uint16_t length
)
{
    for (uint16_t i = 0; i < length; i++)
    {
        while (!(USART1->SR & USART_SR_TXE))
        {
        }

        USART1->DR = data[i];
    }

    while (!(USART1->SR & USART_SR_TC))
    {
    }
}

static void UART1_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin =
        GPIO_PIN_9 |
        GPIO_PIN_10;

    gpio.Mode =
        GPIO_MODE_AF_PP;

    gpio.Pull =
        GPIO_NOPULL;

    gpio.Speed =
        GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Alternate =
        GPIO_AF7_USART1;

    HAL_GPIO_Init(
        GPIOA,
        &gpio
    );

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(
        &huart1
    );

    HAL_NVIC_SetPriority(
        USART1_IRQn,
        1,
        0
    );

    HAL_NVIC_EnableIRQ(
        USART1_IRQn
    );
}

static void UART2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin =
        GPIO_PIN_2 |
        GPIO_PIN_3;

    gpio.Mode =
        GPIO_MODE_AF_PP;

    gpio.Pull =
        GPIO_NOPULL;

    gpio.Speed =
        GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Alternate =
        GPIO_AF7_USART2;

    HAL_GPIO_Init(
        GPIOA,
        &gpio
    );

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(
        &huart2
    );

    HAL_NVIC_SetPriority(
        USART2_IRQn,
        1,
        0
    );

    HAL_NVIC_EnableIRQ(
        USART2_IRQn
    );
}