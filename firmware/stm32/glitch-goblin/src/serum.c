#include "serum.h"

static uint16_t Serum_CRC16_Update(
    uint16_t crc,
    uint8_t byte
)
{
    crc ^= ((uint16_t)byte << 8);

    for (uint8_t i = 0; i < 8; i++)
    {
        if (crc & 0x8000)
        {
            crc = (crc << 1) ^ 0x1021;
        }
        else
        {
            crc <<= 1;
        }
    }

    return crc;
}

void SerumParser_Init(SerumParser *parser)
{
    parser->state = SERUM_WAIT_SYNC_1;
    parser->payload_index = 0;
    parser->calculated_crc = 0xFFFF;
    parser->received_crc = 0;
}

SerumParseResult SerumParser_ProcessByte(
    SerumParser *parser,
    uint8_t byte
)
{
    switch (parser->state)
    {
        case SERUM_WAIT_SYNC_1:
        {
            if (byte == SERUM_SYNC_1)
            {
                parser->state = SERUM_WAIT_SYNC_2;
            }

            break;
        }

        case SERUM_WAIT_SYNC_2:
        {
            if (byte == SERUM_SYNC_2)
            {
                parser->calculated_crc = 0xFFFF;
                parser->received_crc = 0;
                parser->state = SERUM_READ_VERSION;
            }
            else if (byte == SERUM_SYNC_1)
            {
                parser->state = SERUM_WAIT_SYNC_2;
            }
            else
            {
                parser->state = SERUM_WAIT_SYNC_1;
            }

            break;
        }

        case SERUM_READ_VERSION:
        {
            parser->packet.version = byte;

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            if (byte != SERUM_VERSION)
            {
                parser->state = SERUM_WAIT_SYNC_1;
                return SERUM_PARSE_FORMAT_ERROR;
            }

            parser->state = SERUM_READ_TYPE;

            break;
        }

        case SERUM_READ_TYPE:
        {
            parser->packet.type = byte;

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            parser->state = SERUM_READ_LENGTH_HIGH;

            break;
        }

        case SERUM_READ_LENGTH_HIGH:
        {
            parser->packet.length =
                ((uint16_t)byte << 8);

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            parser->state = SERUM_READ_LENGTH_LOW;

            break;
        }

        case SERUM_READ_LENGTH_LOW:
        {
            parser->packet.length |= byte;

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            if (parser->packet.length >
                SERUM_MAX_PAYLOAD_SIZE)
            {
                parser->state = SERUM_WAIT_SYNC_1;
                return SERUM_PARSE_FORMAT_ERROR;
            }

            parser->state =
                SERUM_READ_SEQUENCE_HIGH;

            break;
        }

        case SERUM_READ_SEQUENCE_HIGH:
        {
            parser->packet.sequence =
                ((uint16_t)byte << 8);

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            parser->state =
                SERUM_READ_SEQUENCE_LOW;

            break;
        }

        case SERUM_READ_SEQUENCE_LOW:
        {
            parser->packet.sequence |= byte;

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            parser->payload_index = 0;

            if (parser->packet.length == 0)
            {
                parser->state =
                    SERUM_READ_CRC_HIGH;
            }
            else
            {
                parser->state =
                    SERUM_READ_PAYLOAD;
            }

            break;
        }

        case SERUM_READ_PAYLOAD:
        {
            parser->packet.payload[
                parser->payload_index
            ] = byte;

            parser->calculated_crc =
                Serum_CRC16_Update(
                    parser->calculated_crc,
                    byte
                );

            parser->payload_index++;

            if (parser->payload_index >=
                parser->packet.length)
            {
                parser->state =
                    SERUM_READ_CRC_HIGH;
            }

            break;
        }

        case SERUM_READ_CRC_HIGH:
        {
            parser->received_crc =
                ((uint16_t)byte << 8);

            parser->state =
                SERUM_READ_CRC_LOW;

            break;
        }

        case SERUM_READ_CRC_LOW:
        {
            parser->received_crc |= byte;

            parser->packet.crc =
                parser->received_crc;

            parser->state =
                SERUM_WAIT_SYNC_1;

            if (parser->calculated_crc ==
                parser->received_crc)
            {
                return SERUM_PARSE_PACKET_READY;
            }

            return SERUM_PARSE_CRC_ERROR;
        }

        default:
        {
            parser->state = SERUM_WAIT_SYNC_1;
            return SERUM_PARSE_FORMAT_ERROR;
        }
    }

    return SERUM_PARSE_IN_PROGRESS;
}