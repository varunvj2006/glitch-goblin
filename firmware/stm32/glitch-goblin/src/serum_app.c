#include "serum_app.h"
#include "serum.h"
#include "ring_buffer.h"
#include "board.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


static volatile uint8_t uart2_rx_byte;

static RingBuffer esp32_rx_buffer;
static RingBuffer pc_rx_buffer;

static SerumParser esp32_serum_parser;
static SerumParser pc_serum_parser;


static uint16_t esp32_last_sequence = 0;
static uint8_t esp32_sequence_initialized = 0;

static uint16_t pc_last_sequence = 0;
static uint8_t pc_sequence_initialized = 0;


static uint32_t serum_valid_packets = 0;
static uint32_t serum_crc_errors = 0;
static uint32_t serum_duplicates = 0;
static uint32_t serum_processed_packets = 0;


static uint8_t pc_drop_next_ack = 1;


typedef enum
{
    SERUM_LINK_ESP32,
    SERUM_LINK_PC
} SerumLink;


static void ProcessEsp32Serum(void);
static void ProcessPcSerum(void);

static void SendSerumAck(
    SerumLink link,
    uint16_t sequence
);

static void SendSerumTelemetry(
    SerumLink link,
    uint16_t sequence
);

static void SendSerumBuffer(
    SerumLink link,
    const uint8_t *buffer,
    uint16_t length
);

static void WriteU32BE(
    uint8_t *buffer,
    uint32_t value
);


void SerumApp_Init(void)
{
    RingBuffer_Init(
        &esp32_rx_buffer
    );

    RingBuffer_Init(
        &pc_rx_buffer
    );

    SerumParser_Init(
        &esp32_serum_parser
    );

    SerumParser_Init(
        &pc_serum_parser
    );

    USART1->CR1 |= USART_CR1_RXNEIE;

    HAL_UART_Receive_IT(
        &huart2,
        (uint8_t *)&uart2_rx_byte,
        1
    );
}


void SerumApp_Process(void)
{
    ProcessEsp32Serum();
    ProcessPcSerum();
}


static void ProcessEsp32Serum(void)
{
    uint8_t byte;

    while (
        RingBuffer_Pop(
            &esp32_rx_buffer,
            &byte
        )
    )
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
            uint16_t sequence =
                esp32_serum_parser.packet.sequence;

            bool stats_requested = false;

            if (
                esp32_serum_parser.packet.type ==
                    SERUM_MSG_COMMAND &&
                esp32_serum_parser.packet.length >= 1 &&
                esp32_serum_parser.packet.payload[0] ==
                    SERUM_CMD_GET_STATS
            )
            {
                stats_requested = true;
            }

            if (stats_requested)
            {
                SendSerumAck(
                    SERUM_LINK_ESP32,
                    sequence
                );

                SendSerumTelemetry(
                    SERUM_LINK_ESP32,
                    sequence
                );

                continue;
            }

            serum_valid_packets++;

            bool duplicate = false;

            if (
                esp32_sequence_initialized &&
                sequence ==
                    esp32_last_sequence
            )
            {
                duplicate = true;
            }

            if (!duplicate)
            {
                esp32_last_sequence =
                    sequence;

                esp32_sequence_initialized =
                    1;

                serum_processed_packets++;

                if (
                    esp32_serum_parser.packet.type ==
                        SERUM_MSG_COMMAND &&
                    esp32_serum_parser.packet.length >= 1
                )
                {
                    uint8_t command =
                        esp32_serum_parser.packet.payload[0];

                    if (
                        command ==
                        SERUM_CMD_TOGGLE_LED
                    )
                    {
                        Board_LED_Toggle();
                    }
                }

                const char *msg =
                    "ESP32 SERUM: NEW packet\r\n";

                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)msg,
                    strlen(msg),
                    HAL_MAX_DELAY
                );
            }
            else
            {
                serum_duplicates++;

                const char *msg =
                    "ESP32 SERUM: DUPLICATE packet ignored\r\n";

                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)msg,
                    strlen(msg),
                    HAL_MAX_DELAY
                );
            }

            SendSerumAck(
                SERUM_LINK_ESP32,
                sequence
            );
        }

        else if (
            result ==
            SERUM_PARSE_CRC_ERROR
        )
        {
            serum_crc_errors++;

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

    while (
        RingBuffer_Pop(
            &pc_rx_buffer,
            &byte
        )
    )
    {
        SerumParseResult result =
            SerumParser_ProcessByte(
                &pc_serum_parser,
                byte
            );

        if (
            result ==
            SERUM_PARSE_PACKET_READY
        )
        {
            uint16_t sequence =
                pc_serum_parser.packet.sequence;

            bool duplicate = false;

            if (
                pc_sequence_initialized &&
                sequence ==
                    pc_last_sequence
            )
            {
                duplicate = true;
            }

            if (!duplicate)
            {
                pc_last_sequence =
                    sequence;

                pc_sequence_initialized =
                    1;

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
                        Board_LED_Toggle();
                    }
                }
            }

            if (pc_drop_next_ack)
            {
                pc_drop_next_ack = 0;
            }
            else
            {
                SendSerumAck(
                    SERUM_LINK_PC,
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


static void SendSerumBuffer(
    SerumLink link,
    const uint8_t *buffer,
    uint16_t length
)
{
    if (length == 0)
    {
        return;
    }

    if (link == SERUM_LINK_ESP32)
    {
        Board_UART1_Write(
            buffer,
            length
        );
    }
    else if (link == SERUM_LINK_PC)
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)buffer,
            length,
            HAL_MAX_DELAY
        );
    }
}


static void SendSerumAck(
    SerumLink link,
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

    SendSerumBuffer(
        link,
        tx_buffer,
        tx_length
    );
}


static void WriteU32BE(
    uint8_t *buffer,
    uint32_t value
)
{
    buffer[0] =
        (uint8_t)(value >> 24);

    buffer[1] =
        (uint8_t)(value >> 16);

    buffer[2] =
        (uint8_t)(value >> 8);

    buffer[3] =
        (uint8_t)value;
}


static void SendSerumTelemetry(
    SerumLink link,
    uint16_t sequence
)
{
    SerumPacket packet = {0};

    packet.version =
        SERUM_VERSION;

    packet.type =
        SERUM_MSG_TELEMETRY;

    packet.length = 16;

    packet.sequence =
        sequence;

    WriteU32BE(
        &packet.payload[0],
        serum_valid_packets
    );

    WriteU32BE(
        &packet.payload[4],
        serum_crc_errors
    );

    WriteU32BE(
        &packet.payload[8],
        serum_duplicates
    );

    WriteU32BE(
        &packet.payload[12],
        serum_processed_packets
    );

    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            &packet,
            tx_buffer,
            sizeof(tx_buffer)
        );

    SendSerumBuffer(
        link,
        tx_buffer,
        tx_length
    );
}


void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE)
    {
        uint8_t byte =
            (uint8_t)USART1->DR;

        RingBuffer_Push(
            &esp32_rx_buffer,
            byte
        );
    }
}


void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(
        &huart2
    );
}


void HAL_UART_RxCpltCallback(
    UART_HandleTypeDef *huart
)
{
    if (huart->Instance == USART2)
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