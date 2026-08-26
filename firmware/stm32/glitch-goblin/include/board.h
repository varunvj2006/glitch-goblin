#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx_hal.h"

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

void Board_Init(void);
void Board_LED_Toggle(void);

#endif