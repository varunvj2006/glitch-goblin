#include "stm32f4xx_hal.h"
#include "ring_buffer.h"
#include "serum.h"
#include <string.h>

#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

UART_HandleTypeDef huart2;

volatile uint8_t uart_rx_byte;
RingBuffer uart_rx_buffer;
SerumParser serum_parser;

static void LED_Init(void);
static void UART2_Init(void);

int main(void)
{
    HAL_Init();
    RingBuffer_Init(&uart_rx_buffer);
    SerumParser_Init(&serum_parser);
    LED_Init();
    UART2_Init();

    HAL_UART_Receive_IT(
     &huart2,
    (uint8_t *)&uart_rx_byte,
    1
);

while (1)
{
    uint8_t byte;

    while (RingBuffer_Pop(&uart_rx_buffer, &byte))
    {
        SerumParseResult result =
            SerumParser_ProcessByte(
                &serum_parser,
                byte
            );

    if (result == SERUM_PARSE_PACKET_READY)
    {
        SerumPacket ack_packet = {0};

        ack_packet.version =
            SERUM_VERSION;

        ack_packet.type =
            SERUM_MSG_ACK;

        ack_packet.length = 0;

        ack_packet.sequence =
            serum_parser.packet.sequence;

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
                &huart2,
                tx_buffer,
                tx_length,
                HAL_MAX_DELAY
            );
        }
    }

        else if (result == SERUM_PARSE_CRC_ERROR)
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

        else if (result == SERUM_PARSE_FORMAT_ERROR)
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
}



static void LED_Init(void){
    __HAL_RCC_GPIOA_CLK_ENABLE();  //turns on the hardware clock for the gpio

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LED_PORT, &gpio);
}
static void UART2_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};


    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;    //pa2 and pa3 for tx rx
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200; //comm speed
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&huart2);   

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);   //interrupt controller
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
void SysTick_Handler(void)
{
    HAL_IncTick();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        RingBuffer_Push(
            &uart_rx_buffer,
            uart_rx_byte
        );

        /*
         * Immediately get ready for the next byte.
         */
        HAL_UART_Receive_IT(
            &huart2,
            (uint8_t *)&uart_rx_byte,
            1
        );
   }
}