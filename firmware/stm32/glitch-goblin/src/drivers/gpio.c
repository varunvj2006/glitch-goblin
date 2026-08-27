#include "drivers/gpio.h"

void GPIO_EnableClock(
    GPIO_TypeDef *port
)
{
    if (port == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    }
    else if (port == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    }
    else if (port == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    }
}

void GPIO_SetMode(
    GPIO_TypeDef *port,
    uint8_t pin,
    GPIO_Mode mode
)
{
    port->MODER &= ~(3U << (pin * 2));

    port->MODER |=
        ((uint32_t)mode << (pin * 2));
}

void GPIO_Write(
    GPIO_TypeDef *port,
    uint8_t pin,
    uint8_t state
)
{
    if (state)
    {
        port->ODR |= (1U << pin);
    }
    else
    {
        port->ODR &= ~(1U << pin);
    }
}

void GPIO_Toggle(
    GPIO_TypeDef *port,
    uint8_t pin
)
{
    port->ODR ^= (1U << pin);
}