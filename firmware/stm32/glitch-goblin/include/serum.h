#ifndef SERUM_H
#define SERUM_H

#include <stdint.h>
#include <stdbool.h>

#define SERUM_SYNC_1            0x53
#define SERUM_SYNC_2            0x45
#define SERUM_VERSION           0x01

#define SERUM_MAX_PAYLOAD_SIZE  64

#define SERUM_HEADER_SIZE       8
#define SERUM_CRC_SIZE          2


typedef enum
{
    SERUM_MSG_PING       = 0x01,
    SERUM_MSG_COMMAND    = 0x02,
    SERUM_MSG_TELEMETRY  = 0x03,
    SERUM_MSG_ERROR      = 0x04,
    SERUM_MSG_ACK        = 0x05,
    SERUM_MSG_NACK       = 0x06

} SerumMessageType;


typedef struct
{
    uint8_t version;
    uint8_t type;

    uint16_t length;
    uint16_t sequence;

    uint8_t payload[SERUM_MAX_PAYLOAD_SIZE];

    uint16_t crc;

} SerumPacket;


typedef enum
{
    SERUM_CMD_TOGGLE_LED = 0x01
} SerumCommand;

typedef enum
{
    SERUM_WAIT_SYNC_1,
    SERUM_WAIT_SYNC_2,

    SERUM_READ_VERSION,
    SERUM_READ_TYPE,

    SERUM_READ_LENGTH_HIGH,
    SERUM_READ_LENGTH_LOW,

    SERUM_READ_SEQUENCE_HIGH,
    SERUM_READ_SEQUENCE_LOW,

    SERUM_READ_PAYLOAD,

    SERUM_READ_CRC_HIGH,
    SERUM_READ_CRC_LOW

} SerumParserState;


typedef enum
{
    SERUM_PARSE_IN_PROGRESS,
    SERUM_PARSE_PACKET_READY,
    SERUM_PARSE_CRC_ERROR,
    SERUM_PARSE_FORMAT_ERROR

} SerumParseResult;


typedef struct
{
    SerumParserState state;

    SerumPacket packet;

    uint16_t payload_index;

    uint16_t calculated_crc;
    uint16_t received_crc;

} SerumParser;


void SerumParser_Init(
    SerumParser *parser
);


SerumParseResult SerumParser_ProcessByte(
    SerumParser *parser,
    uint8_t byte
);


uint16_t Serum_EncodePacket(
    const SerumPacket *packet,
    uint8_t *buffer,
    uint16_t buffer_size
);


#endif