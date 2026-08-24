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


static uint16_t Serum_CRC16(
    const uint8_t *data,
    uint16_t length
)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc = Serum_CRC16_Update(
            crc,
            data[i]
        );
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


uint16_t Serum_EncodePacket(
    const SerumPacket *packet,
    uint8_t *buffer,
    uint16_t buffer_size
)
{
    uint16_t total_size =
        SERUM_HEADER_SIZE +
        packet->length +
        SERUM_CRC_SIZE;

    if (packet->length > SERUM_MAX_PAYLOAD_SIZE)
    {
        return 0;
    }

    if (buffer_size < total_size)
    {
        return 0;
    }

    buffer[0] = SERUM_SYNC_1;
    buffer[1] = SERUM_SYNC_2;

    buffer[2] = packet->version;
    buffer[3] = packet->type;

    buffer[4] =
        (uint8_t)(packet->length >> 8);

    buffer[5] =
        (uint8_t)(packet->length & 0xFF);

    buffer[6] =
        (uint8_t)(packet->sequence >> 8);

    buffer[7] =
        (uint8_t)(packet->sequence & 0xFF);

    for (uint16_t i = 0;
         i < packet->length;
         i++)
    {
        buffer[8 + i] =
            packet->payload[i];
    }

    uint16_t crc = Serum_CRC16(
        &buffer[2],
        6 + packet->length
    );

    buffer[8 + packet->length] =
        (uint8_t)(crc >> 8);

    buffer[9 + packet->length] =
        (uint8_t)(crc & 0xFF);

    return total_size;
}