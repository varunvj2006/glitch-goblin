#include <Arduino.h>
#include "serum.h"

HardwareSerial SerumUART(2);

SerumParser serum_parser;

void SendSerumPing(
    uint16_t sequence,
    bool corrupt
)
{
    SerumPacket ping = {0};

    ping.version = SERUM_VERSION;
    ping.type = SERUM_MSG_PING;
    ping.length = 0;
    ping.sequence = sequence;

    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            &ping,
            tx_buffer,
            sizeof(tx_buffer)
        );

    if (tx_length == 0)
    {
        return;
    }

    if (corrupt)
    {
        tx_buffer[7] ^= 0x01;
    }

    SerumUART.write(
        tx_buffer,
        tx_length
    );

    Serial.print("PING #");
    Serial.print(sequence);

    if (corrupt)
    {
        Serial.println(" sent [CORRUPTED]");
    }
    else
    {
        Serial.println(" sent [VALID]");
    }
}

void setup()
{
    Serial.begin(115200);

    SerumUART.begin(
        115200,
        SERIAL_8N1,
        16,
        17
    );

    SerumParser_Init(
        &serum_parser
    );

    delay(1000);

    Serial.println("ESP32 ready");

    SendSerumPing(
        1,
        false
    );

    delay(1000);

    SendSerumPing(
        2,
        true
    );
}

void loop()
{
    while (SerumUART.available())
    {
        uint8_t byte =
            SerumUART.read();

        SerumParseResult result =
            SerumParser_ProcessByte(
                &serum_parser,
                byte
            );

        if (
            result ==
            SERUM_PARSE_PACKET_READY
        )
        {
            Serial.println(
                "SERUM packet received"
            );

            Serial.print(
                "Type: "
            );

            if (
                serum_parser.packet.type ==
                SERUM_MSG_ACK
            )
            {
                Serial.println(
                    "ACK"
                );
            }
            else
            {
                Serial.print(
                    "0x"
                );

                Serial.println(
                    serum_parser.packet.type,
                    HEX
                );
            }

            Serial.print(
                "Sequence: "
            );

            Serial.println(
                serum_parser.packet.sequence
            );

            Serial.println(
                "CRC: VALID"
            );
        }

        else if (
            result ==
            SERUM_PARSE_CRC_ERROR
        )
        {
            Serial.println(
                "SERUM CRC ERROR"
            );
        }

        else if (
            result ==
            SERUM_PARSE_FORMAT_ERROR
        )
        {
            Serial.println(
                "SERUM FORMAT ERROR"
            );
        }
    }
}