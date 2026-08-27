#include "board.h"


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
void Board_UART2_Write(
    const uint8_t *data,
    uint16_t length
)
{
    for (uint16_t i = 0; i < length; i++)
    {
        while (!(USART2->SR & USART_SR_TXE))
        {
        }

        USART2->DR = data[i];
    }

    while (!(USART2->SR & USART_SR_TC))
    {
    }
}
static void UART1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER &= ~(
        (3U << (9 * 2)) |
        (3U << (10 * 2))
    );

    GPIOA->MODER |= (
        (2U << (9 * 2)) |
        (2U << (10 * 2))
    );

    GPIOA->OTYPER &= ~(
        (1U << 9) |
        (1U << 10)
    );

    GPIOA->PUPDR &= ~(
        (3U << (9 * 2)) |
        (3U << (10 * 2))
    );

    GPIOA->OSPEEDR |= (
        (2U << (9 * 2)) |
        (2U << (10 * 2))
    );

    GPIOA->AFR[1] &= ~(
        (0xFU << 4) |
        (0xFU << 8)
    );

    GPIOA->AFR[1] |= (
        (7U << 4) |
        (7U << 8)
    );

    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    USART1->BRR = 0x008B;

    USART1->CR1 |=
        USART_CR1_TE |
        USART_CR1_RE;

    USART1->CR1 |= USART_CR1_UE;


    NVIC_SetPriority(
        USART1_IRQn,
        1
    );

    NVIC_EnableIRQ(
        USART1_IRQn
    );
}
static void UART2_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;  //b1 for usart2

    GPIOA->MODER &= ~(
        (3U << (2 * 2)) |    //reset pa2,3
        (3U << (3 * 2))
    );

    GPIOA->MODER |= (    //set pa2,3 to alt mode
        (2U << (2 * 2)) |
        (2U << (3 * 2))
    );

    GPIOA->OTYPER &= ~(   //reset pa 3,2 to push-pull
        (1U << 2) |
        (1U << 3)
    );

    GPIOA->PUPDR &= ~(   //reset pa2,3 to no pull-up/down
        (3U << (2 * 2)) |
        (3U << (3 * 2))
    );

    GPIOA->OSPEEDR |= (   //set pa2,3 to high peed
        (2U << (2 * 2)) |
        (2U << (3 * 2))
    );

    GPIOA->AFR[0] &= ~(  //reset pa2,3 
        (0xFU << 8) |
        (0xFU << 12)
    );

    GPIOA->AFR[0] |= (  //set pa2,3 for alt7
        (7U << 8) |
        (7U << 12)
    );

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    USART2->BRR = 0x008B;

    USART2->CR1 |=
        USART_CR1_TE |
        USART_CR1_RE;

    USART2->CR1 |= USART_CR1_UE;   //usart2 enable


    NVIC_SetPriority(
        USART2_IRQn,
        1
    );

    NVIC_EnableIRQ(
        USART2_IRQn
    );
}       