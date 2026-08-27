#include "board.h"
#include "drivers/uart.h"
#include "drivers/gpio.h"


static void LED_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);

void Board_Init(void)
{
    LED_Init();

    UART_Init(
        UART_PORT_1,
        115200
    );

    UART_Init(
        UART_PORT_2,
        115200
    );
}

static void LED_Init(void)
{
    GPIO_EnableClock(GPIOA);

    GPIO_SetMode(
        GPIOA,
        5,
        GPIO_PIN_MODE_OUTPUT
    );

    GPIOA->OTYPER &= ~(1U << 5);
    GPIOA->PUPDR &= ~(3U << (5 * 2));
    GPIOA->OSPEEDR &= ~(3U << (5 * 2));
}

void Board_LED_Toggle(void)
{
    GPIO_Toggle(
        GPIOA,
        5
    );
}

void Board_UART1_Write(
    const uint8_t *data,
    uint16_t length
)
{
    UART_Write(
        UART_PORT_1,
        data,
        length
    );
}
void Board_UART2_Write(
    const uint8_t *data,
    uint16_t length
)
{
    UART_Write(
        UART_PORT_2,
        data,
        length
    );
}
