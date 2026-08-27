#include "drivers/uart.h"

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