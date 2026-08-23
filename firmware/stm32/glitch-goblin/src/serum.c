#include "serum.h"


void SerumParser_Init(SerumParser *parser)
{
    parser->state = SERUM_WAIT_SYNC_1;
    parser->payload_index = 0;
}


bool SerumParser_ProcessByte(
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

            if (byte != SERUM_VERSION)
            {
                parser->state = SERUM_WAIT_SYNC_1;
                break;
            }

            parser->state = SERUM_READ_TYPE;

            break;
        }


        case SERUM_READ_TYPE:
        {
            parser->packet.type = byte;

            parser->state = SERUM_READ_LENGTH_HIGH;

            break;
        }


        case SERUM_READ_LENGTH_HIGH:
        {
            parser->packet.length =
                ((uint16_t)byte << 8);

            parser->state = SERUM_READ_LENGTH_LOW;

            break;
        }


        case SERUM_READ_LENGTH_LOW:
        {
            parser->packet.length |= byte;

            if (parser->packet.length >
                SERUM_MAX_PAYLOAD_SIZE)
            {
                parser->state = SERUM_WAIT_SYNC_1;

                break;
            }

            parser->state =
                SERUM_READ_SEQUENCE_HIGH;

            break;
        }


        case SERUM_READ_SEQUENCE_HIGH:
        {
            parser->packet.sequence =
                ((uint16_t)byte << 8);

            parser->state =
                SERUM_READ_SEQUENCE_LOW;

            break;
        }


        case SERUM_READ_SEQUENCE_LOW:
        {
            parser->packet.sequence |= byte;

            parser->payload_index = 0;

            if (parser->packet.length == 0)
            {
                parser->state = SERUM_WAIT_SYNC_1;

                return true;
            }

            parser->state = SERUM_READ_PAYLOAD;

            break;
        }


        case SERUM_READ_PAYLOAD:
        {
            parser->packet.payload[
                parser->payload_index
            ] = byte;

            parser->payload_index++;

            if (parser->payload_index >=
                parser->packet.length)
            {
                parser->state = SERUM_WAIT_SYNC_1;

                return true;
            }

            break;
        }


        default:
        {
            parser->state = SERUM_WAIT_SYNC_1;

            break;
        }
    }

    return false;
}