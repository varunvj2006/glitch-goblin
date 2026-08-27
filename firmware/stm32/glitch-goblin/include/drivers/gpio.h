#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include "stm32f4xx.h"

typedef enum
{
    GPIO_PIN_MODE_INPUT = 0,
    GPIO_PIN_MODE_OUTPUT = 1,
    GPIO_PIN_MODE_ALTERNATE = 2,
    GPIO_PIN_MODE_ANALOG = 3
} GPIO_Mode;

void GPIO_EnableClock(
    GPIO_TypeDef *port
);

void GPIO_SetMode(
    GPIO_TypeDef *port,
    uint8_t pin,
    GPIO_Mode mode
);

void GPIO_Write(
    GPIO_TypeDef *port,
    uint8_t pin,
    uint8_t state
);

void GPIO_Toggle(
    GPIO_TypeDef *port,
    uint8_t pin
);

#endif