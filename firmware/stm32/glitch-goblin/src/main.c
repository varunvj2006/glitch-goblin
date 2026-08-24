#include "stm32f4xx_hal.h"
#include "ring_buffer.h"
#include "serum.h"
#include <string.h>
#include <stdbool.h>

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

volatile uint8_t uart1_rx_byte;
volatile uint8_t uart2_rx_byte;

RingBuffer esp32_rx_buffer;
RingBuffer pc_rx_buffer;

SerumParser esp32_serum_parser;
SerumParser pc_serum_parser;

static uint8_t drop_next_ack = 1;
static uint16_t last_sequence = 0;
static uint8_t sequence_initialized = 0;

static void LED_Init(void);
static void UART1_Init(void);
static void UART2_Init(void);

static void ProcessEsp32Serum(void);
static void ProcessPcSerum(void);

static void SendSerumAck(
    UART_HandleTypeDef *huart,
    uint16_t sequence
);

int main(void)
{
    HAL_Init();

    RingBuffer_Init(&esp32_rx_buffer);
    RingBuffer_Init(&pc_rx_buffer);

    SerumParser_Init(&esp32_serum_parser);
    SerumParser_Init(&pc_serum_parser);

    LED_Init();
    UART1_Init();
    UART2_Init();

    HAL_UART_Receive_IT(
        &huart1,
        (uint8_t *)&uart1_rx_byte,
        1
    );

    HAL_UART_Receive_IT(
        &huart2,
        (uint8_t *)&uart2_rx_byte,
        1
    );

    while (1)
    {
        ProcessEsp32Serum();
        ProcessPcSerum();
    }
}

static void ProcessEsp32Serum(void)
{
    uint8_t byte;

    while (RingBuffer_Pop(
        &esp32_rx_buffer,
        &byte
    ))
    {
        SerumParseResult result =
            SerumParser_ProcessByte(
                &esp32_serum_parser,
                byte
            );

        if (
            result ==
            SERUM_PARSE_PACKET_READY
        )
        {
            SendSerumAck(
                &huart1,
                esp32_serum_parser.packet.sequence
            );
        }

        else if (
            result ==
            SERUM_PARSE_CRC_ERROR
        )
        {
            const char *msg =
                "ESP32 SERUM: CRC fault detected\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        else if (
            result ==
            SERUM_PARSE_FORMAT_ERROR
        )
        {
            const char *msg =
                "ESP32 SERUM: format error\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }
    }
}

static void ProcessPcSerum(void)
{
    uint8_t byte;

    while (RingBuffer_Pop(
        &pc_rx_buffer,
        &byte
    ))
    {
        SerumParseResult result =
            SerumParser_ProcessByte(
                &pc_serum_parser,
                byte
            );

        if (result == SERUM_PARSE_PACKET_READY)
        {
            uint16_t sequence =
                pc_serum_parser.packet.sequence;

            bool duplicate = false;

            if (
                sequence_initialized &&
                sequence == last_sequence
            )
            {
                duplicate = true;
            }

            if (!duplicate)
            {
                last_sequence = sequence;
                sequence_initialized = 1;

                if (
                    pc_serum_parser.packet.type ==
                        SERUM_MSG_COMMAND &&
                    pc_serum_parser.packet.length >= 1
                )
                {
                    uint8_t command =
                        pc_serum_parser.packet.payload[0];

                    if (
                        command ==
                        SERUM_CMD_TOGGLE_LED
                    )
                    {
                        HAL_GPIO_TogglePin(
                            LED_PORT,
                            LED_PIN
                        );
                    }
                }
            }

            if (drop_next_ack)
            {
                drop_next_ack = 0;
            }
            else
            {
                SendSerumAck(
                    &huart2,
                    sequence
                );
            }
        }

        else if (
            result ==
            SERUM_PARSE_CRC_ERROR
        )
        {
            const char *msg =
                "SERUM: CRC FAILED - packet rejected >:(\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }

        else if (
            result ==
            SERUM_PARSE_FORMAT_ERROR
        )
        {
            const char *msg =
                "SERUM: malformed packet\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)msg,
                strlen(msg),
                HAL_MAX_DELAY
            );
        }
    }
}

static void SendSerumAck(
    UART_HandleTypeDef *huart,
    uint16_t sequence
)
{
    SerumPacket ack_packet = {0};

    ack_packet.version =
        SERUM_VERSION;

    ack_packet.type =
        SERUM_MSG_ACK;

    ack_packet.length = 0;

    ack_packet.sequence =
        sequence;

    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            &ack_packet,
            tx_buffer,
            sizeof(tx_buffer)
        );

    if (tx_length > 0)
    {
        HAL_UART_Transmit(
            huart,
            tx_buffer,
            tx_length,
            HAL_MAX_DELAY
        );
    }
}

static void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        LED_PORT,
        &gpio
    );
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

    huart1.Instance =
        USART1;

    huart1.Init.BaudRate =
        115200;

    huart1.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart1.Init.StopBits =
        UART_STOPBITS_1;

    huart1.Init.Parity =
        UART_PARITY_NONE;

    huart1.Init.Mode =
        UART_MODE_TX_RX;

    huart1.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart1.Init.OverSampling =
        UART_OVERSAMPLING_16;

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

    huart2.Instance =
        USART2;

    huart2.Init.BaudRate =
        115200;

    huart2.Init.WordLength =
        UART_WORDLENGTH_8B;

    huart2.Init.StopBits =
        UART_STOPBITS_1;

    huart2.Init.Parity =
        UART_PARITY_NONE;

    huart2.Init.Mode =
        UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl =
        UART_HWCONTROL_NONE;

    huart2.Init.OverSampling =
        UART_OVERSAMPLING_16;

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

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(
        &huart1
    );
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(
        &huart2
    );
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance == USART1)
    {
        RingBuffer_Push(
            &esp32_rx_buffer,
            uart1_rx_byte
        );

        HAL_UART_Receive_IT(
            &huart1,
            (uint8_t *)&uart1_rx_byte,
            1
        );
    }

    else if (
        huart->Instance ==
        USART2
    )
    {
        RingBuffer_Push(
            &pc_rx_buffer,
            uart2_rx_byte
        );

        HAL_UART_Receive_IT(
            &huart2,
            (uint8_t *)&uart2_rx_byte,
            1
        );
    }
}