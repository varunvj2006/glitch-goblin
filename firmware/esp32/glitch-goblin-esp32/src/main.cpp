#include <Arduino.h>
#include "serum.h"

HardwareSerial SerumUART(2);

SerumParser serum_parser;
typedef enum
{
    FAULT_NONE,
    FAULT_BAD_CRC,
    FAULT_DROP,
    FAULT_DUPLICATE,
    FAULT_DELAY

} FaultMode;


void SendSerumPing(
    uint16_t sequence,
    FaultMode fault
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
        Serial.println("Packet encoding failed");
        return;
    }

    if (fault == FAULT_BAD_CRC)
    {
        tx_buffer[7] ^= 0x01;

        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" sent [BAD CRC]");

        SerumUART.write(
            tx_buffer,
            tx_length
        );

        return;
    }

    if (fault == FAULT_DROP)
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" [DROPPED]");

        return;
    }

    if (fault == FAULT_DELAY)
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(" [DELAYED]");

        delay(2000);
    }

    SerumUART.write(
        tx_buffer,
        tx_length
    );

    Serial.print("PING #");
    Serial.print(sequence);

    if (fault == FAULT_DUPLICATE)
    {
        Serial.println(" sent [DUPLICATE]");

        delay(50);

        SerumUART.write(
            tx_buffer,
            tx_length
        );
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
        FAULT_DUPLICATE
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