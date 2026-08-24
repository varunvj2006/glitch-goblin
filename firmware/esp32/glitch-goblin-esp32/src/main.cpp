#include <Arduino.h>
#include "serum.h"

HardwareSerial SerumUART(2);

SerumParser serum_parser;
uint16_t next_sequence = 1;

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
void RunFaultTest(
    FaultMode fault,
    bool expect_ack
)
{
    uint16_t sequence =
        next_sequence++;

    SendSerumPing(
        sequence,
        fault
    );

    bool ack =
        WaitForAck(
            sequence,
            500
        );

    bool passed =
        (ack == expect_ack);

    if (passed)
    {
        Serial.println("PASS");
    }
    else
    {
        Serial.println("FAIL");
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

    Serial.setTimeout(2000);

    delay(1000);

    Serial.println();
    Serial.println("=== GLITCH GOBLIN ===");
    Serial.println("Commands:");
    Serial.println("normal");
    Serial.println("crc");
    Serial.println("drop");
    Serial.println("duplicate");
    Serial.println("delay");
    Serial.println("stats");
    Serial.println();
}


void loop()
{
    if (!Serial.available())
    {
        return;
    }

    String command =
        Serial.readStringUntil('\n');

    command.trim();
    command.toLowerCase();

    if (command == "normal")
    {
        RunFaultTest(
            FAULT_NONE,
            true
        );
    }

    else if (command == "crc")
    {
        RunFaultTest(
            FAULT_BAD_CRC,
            false
        );
    }

    else if (command == "drop")
    {
        RunFaultTest(
            FAULT_DROP,
            false
        );
    }

    else if (command == "duplicate")
    {
        RunFaultTest(
            FAULT_DUPLICATE,
            true
        );
    }

    else if (command == "delay")
    {
        RunFaultTest(
            FAULT_DELAY,
            true
        );
    }

    else if (command == "stats")
    {
        uint16_t sequence =
            next_sequence++;

        SendStatsRequest(
            sequence
        );

        if (!WaitForAck(
                sequence,
                500
            ))
        {
            Serial.println(
                "Stats ACK failed"
            );

            return;
        }

        if (!WaitForTelemetry(
                sequence,
                1000
            ))
        {
            Serial.println(
                "Telemetry timeout"
            );
        }
    }

    else
    {
        Serial.print(
            "Unknown command: "
        );

        Serial.println(
            command
        );
    }

    Serial.println();
}