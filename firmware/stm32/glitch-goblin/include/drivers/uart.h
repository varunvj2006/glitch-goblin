#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f4xx.h"

typedef enum
{
    UART_PORT_1,
    UART_PORT_2
} UART_Port;

void UART_Init(
    UART_Port port,
    uint32_t baud_rate
);

void UART_Write(
    UART_Port port,
    const uint8_t *data,
    uint16_t length
);

uint8_t UART_ReadByte(
    UART_Port port
);

void UART_EnableRxInterrupt(
    UART_Port port
);

#endif