#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx_hal.h"

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA


void Board_UART1_Write(
    const uint8_t *data,
    uint16_t length
);
void Board_UART2_Write(
    const uint8_t *data,
    uint16_t length
);
void Board_Init(void);
void Board_LED_Toggle(void);

#endif