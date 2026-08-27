#include <Arduino.h>

#include "serum_link.h"
#include "fault_engine.h"
#include "stats.h"

static HardwareSerial SerumUART(2);

static SerumParser serum_parser;

static uint16_t next_sequence =
    1;

static uint8_t retry_max_attempts =
    3;

static uint32_t retry_timeout_ms =
    500;

static uint32_t ReadU32BE(
    const uint8_t *buffer
)
{
    return
        ((uint32_t)buffer[0] << 24) |
        ((uint32_t)buffer[1] << 16) |
        ((uint32_t)buffer[2] << 8) |
        ((uint32_t)buffer[3]);
}

void SerumLink_Init(void)
{
    SerumUART.begin(
        115200,
        SERIAL_8N1,
        16,
        17
    );

    SerumParser_Init(
        &serum_parser
    );
}

void SerumLink_WriteRaw(
    const uint8_t *buffer,
    uint16_t length
)
{
    SerumUART.write(
        buffer,
        length
    );
}

uint16_t SerumLink_NextSequence(void)
{
    return
        next_sequence++;
}

uint8_t SerumLink_GetRetryMaxAttempts(void)
{
    return
        retry_max_attempts;
}

uint32_t SerumLink_GetRetryTimeoutMs(void)
{
    return
        retry_timeout_ms;
}

bool SerumLink_WaitForAck(
    uint16_t sequence,
    uint32_t timeout_ms
)
{
    uint32_t start =
        millis();

    while (
        millis() - start <
        timeout_ms
    )
    {
        while (
            SerumUART.available()
        )
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

bool SerumLink_WaitForTelemetry(
    uint16_t sequence,
    uint32_t timeout_ms
)
{
    uint32_t start =
        millis();

    while (
        millis() - start <
        timeout_ms
    )
    {
        while (
            SerumUART.available()
        )
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
                    serum_parser.packet.length >=
                        16
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

                    Serial.println(
                        "=== STM32 TELEMETRY ==="
                    );

                    Serial.print(
                        "Valid packets:     "
                    );

                    Serial.println(
                        valid_packets
                    );

                    Serial.print(
                        "CRC errors:        "
                    );

                    Serial.println(
                        crc_errors
                    );

                    Serial.print(
                        "Duplicates:        "
                    );

                    Serial.println(
                        duplicates
                    );

                    Serial.print(
                        "Processed packets: "
                    );

                    Serial.println(
                        processed_packets
                    );

                    Serial.println(
                        "======================="
                    );

                    return true;
                }
            }
        }

        delay(1);
    }

    return false;
}

void SerumLink_SendPing(
    uint16_t sequence,
    FaultMode fault
)
{
    SerumPacket ping = {0};

    ping.version =
        SERUM_VERSION;

    ping.type =
        SERUM_MSG_PING;

    ping.length =
        0;

    ping.sequence =
        sequence;

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

    if (
        tx_length == 0
    )
    {
        Serial.println(
            "Packet encoding failed"
        );

        return;
    }

    Serial.print("PING #");
    Serial.print(sequence);

    if (
        fault ==
        FAULT_BAD_CRC
    )
    {
        Serial.println(
            " sent [BAD CRC]"
        );
    }

    else if (
        fault ==
        FAULT_DROP
    )
    {
        Serial.println(
            " [DROPPED]"
        );
    }

    else if (
        fault ==
        FAULT_DUPLICATE
    )
    {
        Serial.println(
            " sent [DUPLICATE]"
        );
    }

    else if (
        fault ==
        FAULT_DELAY
    )
    {
        Serial.println(
            " [DELAYED]"
        );
    }

    else
    {
        Serial.println(
            " sent [VALID]"
        );
    }

    FaultEngine_SendWithFault(
        tx_buffer,
        tx_length,
        fault
    );
}

void SerumLink_SendStatsRequest(
    uint16_t sequence
)
{
    SerumPacket packet = {0};

    packet.version =
        SERUM_VERSION;

    packet.type =
        SERUM_MSG_COMMAND;

    packet.length =
        1;

    packet.sequence =
        sequence;

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

    if (
        tx_length > 0
    )
    {
        SerumLink_WriteRaw(
            tx_buffer,
            tx_length
        );

        Serial.println();

        Serial.println(
            "Requesting STM32 statistics..."
        );
    }
}

bool SerumLink_SendReliablePacket(
    const SerumPacket *packet,
    uint8_t max_attempts,
    uint32_t timeout_ms
)
{
    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            packet,
            tx_buffer,
            sizeof(tx_buffer)
        );

    if (
        tx_length == 0
    )
    {
        Serial.println(
            "Packet encoding failed"
        );

        Stats_RecordFailure();

        return false;
    }

    for (
        uint8_t attempt = 1;
        attempt <= max_attempts;
        attempt++
    )
    {
        if (
            attempt > 1
        )
        {
            Stats_RecordRetry();
        }

        Serial.print("TX #");

        Serial.print(
            packet->sequence
        );

        Serial.print(
            " attempt "
        );

        Serial.println(
            attempt
        );

        uint32_t start_us =
            micros();

        SerumLink_WriteRaw(
            tx_buffer,
            tx_length
        );

        if (
            SerumLink_WaitForAck(
                packet->sequence,
                timeout_ms
            )
        )
        {
            uint32_t rtt_us =
                micros() -
                start_us;

            Stats_RecordSuccess(
                rtt_us,
                attempt
            );

            Serial.print("ACK #");

            Serial.print(
                packet->sequence
            );

            Serial.println(
                " received"
            );

            Serial.print(
                "RTT: "
            );

            Serial.print(
                rtt_us
            );

            Serial.println(
                " us"
            );

            Serial.print(
                "Delivered after "
            );

            Serial.print(
                attempt
            );

            Serial.println(
                " attempt(s)"
            );

            return true;
        }

        Stats_RecordTimeout();

        Serial.println(
            "ACK timeout"
        );
    }

    Stats_RecordFailure();

    Serial.print(
        "Packet #"
    );

    Serial.print(
        packet->sequence
    );

    Serial.println(
        " delivery failed"
    );

    return false;
}

bool SerumLink_SendReliableChaosPacket(
    const SerumPacket *packet,
    uint8_t max_attempts,
    uint32_t timeout_ms
)
{
    uint8_t tx_buffer[
        SERUM_HEADER_SIZE +
        SERUM_MAX_PAYLOAD_SIZE +
        SERUM_CRC_SIZE
    ];

    uint16_t tx_length =
        Serum_EncodePacket(
            packet,
            tx_buffer,
            sizeof(tx_buffer)
        );

    if (
        tx_length == 0
    )
    {
        Stats_RecordFailure();

        return false;
    }

    for (
        uint8_t attempt = 1;
        attempt <= max_attempts;
        attempt++
    )
    {
        if (
            attempt > 1
        )
        {
            Stats_RecordRetry();
        }

        FaultMode fault =
            FaultEngine_GetRandomFault();

        Stats_RecordFault(
            fault
        );

        Serial.print("TX #");

        Serial.print(
            packet->sequence
        );

        Serial.print(
            " attempt "
        );

        Serial.print(
            attempt
        );

        Serial.print(
            " ["
        );

        Serial.print(
            FaultEngine_GetName(
                fault
            )
        );

        Serial.println(
            "]"
        );

        uint32_t start_us =
            micros();

        FaultEngine_SendWithFault(
            tx_buffer,
            tx_length,
            fault
        );

        if (
            SerumLink_WaitForAck(
                packet->sequence,
                timeout_ms
            )
        )
        {
            uint32_t rtt_us =
                micros() -
                start_us;

            Stats_RecordSuccess(
                rtt_us,
                attempt
            );

            Serial.print(
                "ACK #"
            );

            Serial.print(
                packet->sequence
            );

            Serial.println(
                " received"
            );

            return true;
        }

        Stats_RecordTimeout();

        Serial.println(
            "ACK timeout"
        );
    }

    Stats_RecordFailure();

    Serial.print(
        "Packet #"
    );

    Serial.print(
        packet->sequence
    );

    Serial.println(
        " failed"
    );

    return false;
}