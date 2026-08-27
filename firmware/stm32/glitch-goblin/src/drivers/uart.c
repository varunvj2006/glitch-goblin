#include "drivers/uart.h"
#include "drivers/clock.h"


static USART_TypeDef *UART_GetPeripheral(
    UART_Port port
)
{
    if (port == UART_PORT_1)
    {
        return USART1;
    }

    return USART2;
}


void UART_Write(
    UART_Port port,
    const uint8_t *data,
    uint16_t length
)
{
    USART_TypeDef *uart =
        UART_GetPeripheral(port);

    for (uint16_t i = 0; i < length; i++)
    {
        while (!(uart->SR & USART_SR_TXE))
        {
        }

        uart->DR = data[i];
    }

    while (!(uart->SR & USART_SR_TC))
    {
    }
}


uint8_t UART_ReadByte(
    UART_Port port
)
{
    USART_TypeDef *uart =
        UART_GetPeripheral(port);

    return (uint8_t)uart->DR;
}


void UART_EnableRxInterrupt(
    UART_Port port
)
{
    USART_TypeDef *uart =
        UART_GetPeripheral(port);

    uart->CR1 |= USART_CR1_RXNEIE;
}


void UART_Init(
    UART_Port port,
    uint32_t baud_rate
)
{
    USART_TypeDef *uart =
        UART_GetPeripheral(port);

    uint32_t peripheral_clock;


    if (port == UART_PORT_1)
    {
        RCC->AHB1ENR |=
            RCC_AHB1ENR_GPIOAEN;

        RCC->APB2ENR |=
            RCC_APB2ENR_USART1EN;

        peripheral_clock =
            Clock_GetAPB2Hz();

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

        GPIOA->AFR[1] &= ~(
            (0xFU << 4) |
            (0xFU << 8)
        );

        GPIOA->AFR[1] |= (
            (7U << 4) |
            (7U << 8)
        );

        NVIC_SetPriority(
            USART1_IRQn,
            1
        );

        NVIC_EnableIRQ(
            USART1_IRQn
        );
    }

    else
    {
        RCC->AHB1ENR |=
            RCC_AHB1ENR_GPIOAEN;

        RCC->APB1ENR |=
            RCC_APB1ENR_USART2EN;

        peripheral_clock =
            Clock_GetAPB1Hz();

        GPIOA->MODER &= ~(
            (3U << (2 * 2)) |
            (3U << (3 * 2))
        );

        GPIOA->MODER |= (
            (2U << (2 * 2)) |
            (2U << (3 * 2))
        );

        GPIOA->OTYPER &= ~(
            (1U << 2) |
            (1U << 3)
        );

        GPIOA->PUPDR &= ~(
            (3U << (2 * 2)) |
            (3U << (3 * 2))
        );

        GPIOA->AFR[0] &= ~(
            (0xFU << 8) |
            (0xFU << 12)
        );

        GPIOA->AFR[0] |= (
            (7U << 8) |
            (7U << 12)
        );

        NVIC_SetPriority(
            USART2_IRQn,
            1
        );

        NVIC_EnableIRQ(
            USART2_IRQn
        );
    }


    uart->CR1 = 0;
    uart->CR2 = 0;
    uart->CR3 = 0;

    uart->BRR =
        (peripheral_clock +
         (baud_rate / 2U)) /
        baud_rate;

    uart->CR1 |=
        USART_CR1_TE |
        USART_CR1_RE;

    uart->CR1 |=
        USART_CR1_UE;
}