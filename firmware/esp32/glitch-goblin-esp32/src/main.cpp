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


bool WaitForAck(
    uint16_t sequence,
    uint32_t timeout_ms
)
{
    uint32_t start = millis();

    while (millis() - start < timeout_ms)
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
                if (
                    serum_parser.packet.type ==
                        SERUM_MSG_ACK &&
                    serum_parser.packet.sequence ==
                        sequence
                )
                {
                    return true;
                }
            }
        }

        delay(1);
    }

    return false;
}


uint32_t ReadU32BE(
    const uint8_t *buffer
)
{
    return
        ((uint32_t)buffer[0] << 24) |
        ((uint32_t)buffer[1] << 16) |
        ((uint32_t)buffer[2] << 8) |
        ((uint32_t)buffer[3]);
}


bool WaitForTelemetry(
    uint16_t sequence,
    uint32_t timeout_ms
)
{
    uint32_t start = millis();

    while (millis() - start < timeout_ms)
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
                if (
                    serum_parser.packet.type ==
                        SERUM_MSG_TELEMETRY &&
                    serum_parser.packet.sequence ==
                        sequence &&
                    serum_parser.packet.length >= 16
                )
                {
                    uint32_t valid_packets =
                        ReadU32BE(
                            &serum_parser.packet.payload[0]
                        );

                    uint32_t crc_errors =
                        ReadU32BE(
                            &serum_parser.packet.payload[4]
                        );

                    uint32_t duplicates =
                        ReadU32BE(
                            &serum_parser.packet.payload[8]
                        );

                    uint32_t processed_packets =
                        ReadU32BE(
                            &serum_parser.packet.payload[12]
                        );

                    Serial.println();
                    Serial.println("=== STM32 TELEMETRY ===");

                    Serial.print("Valid packets:     ");
                    Serial.println(valid_packets);

                    Serial.print("CRC errors:        ");
                    Serial.println(crc_errors);

                    Serial.print("Duplicates:        ");
                    Serial.println(duplicates);

                    Serial.print("Processed packets: ");
                    Serial.println(processed_packets);

                    Serial.println("=======================");

                    return true;
                }
            }
        }

        delay(1);
    }

    return false;
}


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
        tx_buffer[tx_length - 1] ^= 0x01;

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


void SendStatsRequest(
    uint16_t sequence
)
{
    SerumPacket packet = {0};

    packet.version = SERUM_VERSION;
    packet.type = SERUM_MSG_COMMAND;
    packet.length = 1;
    packet.sequence = sequence;

    packet.payload[0] =
        SERUM_CMD_GET_STATS;

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

    if (tx_length > 0)
    {
        SerumUART.write(
            tx_buffer,
            tx_length
        );

        Serial.println();
        Serial.println("Requesting STM32 statistics...");
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

    int passed = 0;
    int total = 5;

    Serial.println();
    Serial.println("=== GLITCH GOBLIN SERUM TEST ===");
    Serial.println();


    Serial.println("[1] NORMAL");

    SendSerumPing(
        1,
        FAULT_NONE
    );

    if (WaitForAck(1, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[2] BAD CRC");

    SendSerumPing(
        2,
        FAULT_BAD_CRC
    );

    if (!WaitForAck(2, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[3] DROP");

    SendSerumPing(
        3,
        FAULT_DROP
    );

    if (!WaitForAck(3, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[4] DUPLICATE");

    SendSerumPing(
        4,
        FAULT_DUPLICATE
    );

    if (WaitForAck(4, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }

    delay(500);


    Serial.println();
    Serial.println("[5] DELAY");

    SendSerumPing(
        5,
        FAULT_DELAY
    );

    if (WaitForAck(5, 500))
    {
        Serial.println("PASS");
        passed++;
    }
    else
    {
        Serial.println("FAIL");
    }


    Serial.println();
    Serial.println("==============================");

    Serial.print("RESULTS: ");
    Serial.print(passed);
    Serial.print(" / ");
    Serial.print(total);
    Serial.println(" PASSED");

    Serial.println("==============================");


    delay(500);

    SendStatsRequest(6);

    if (!WaitForAck(6, 500))
    {
        Serial.println(
            "Stats request ACK failed"
        );
        return;
    }

    if (!WaitForTelemetry(6, 1000))
    {
        Serial.println(
            "Telemetry response timed out"
        );
    }
}


void loop()
{
}