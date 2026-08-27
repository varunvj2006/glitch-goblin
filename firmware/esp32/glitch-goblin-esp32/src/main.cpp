#include <Arduino.h>
#include <string.h>
#include "serum.h"

HardwareSerial SerumUART(2);

SerumParser serum_parser;

uint16_t next_sequence = 1;

uint32_t rtt_min_us = UINT32_MAX;
uint32_t rtt_max_us = 0;
uint64_t rtt_total_us = 0;

uint32_t successful_deliveries = 0;
uint32_t failed_deliveries = 0;
uint32_t total_retries = 0;
uint32_t total_timeouts = 0;

uint32_t chaos_normal = 0;
uint32_t chaos_crc = 0;
uint32_t chaos_drop = 0;
uint32_t chaos_duplicate = 0;
uint32_t chaos_delay = 0;

uint8_t retry_max_attempts = 3;
uint32_t retry_timeout_ms = 500;


typedef enum
{
    FAULT_NONE,
    FAULT_BAD_CRC,
    FAULT_DROP,
    FAULT_DUPLICATE,
    FAULT_DELAY
} FaultMode;


void ResetChaosStats(void)
{
    chaos_normal = 0;
    chaos_crc = 0;
    chaos_drop = 0;
    chaos_duplicate = 0;
    chaos_delay = 0;
}


void ResetLinkStats(void)
{
    rtt_min_us = UINT32_MAX;
    rtt_max_us = 0;
    rtt_total_us = 0;

    successful_deliveries = 0;
    failed_deliveries = 0;
    total_retries = 0;
    total_timeouts = 0;
}


void PrintLinkStats(void)
{
    Serial.println();
    Serial.println("=== SERUM LINK STATS ===");

    Serial.print("Successful:  ");
    Serial.println(successful_deliveries);

    Serial.print("Failed:      ");
    Serial.println(failed_deliveries);

    Serial.print("Retries:     ");
    Serial.println(total_retries);

    Serial.print("Timeouts:    ");
    Serial.println(total_timeouts);

    uint32_t total_transactions =
        successful_deliveries +
        failed_deliveries;

    if (total_transactions > 0)
    {
        float delivery_rate =
            ((float)successful_deliveries /
             (float)total_transactions) *
            100.0f;

        Serial.print("Delivery:    ");
        Serial.print(delivery_rate, 2);
        Serial.println("%");
    }

    if (successful_deliveries > 0)
    {
        uint32_t average_rtt =
            (uint32_t)(
                rtt_total_us /
                successful_deliveries
            );

        Serial.print("Min RTT:     ");
        Serial.print(rtt_min_us);
        Serial.println(" us");

        Serial.print("Max RTT:     ");
        Serial.print(rtt_max_us);
        Serial.println(" us");

        Serial.print("Average RTT: ");
        Serial.print(average_rtt);
        Serial.println(" us");
    }

    Serial.println("========================");
}


void PrintChaosStats(void)
{
    Serial.println();
    Serial.println("=== FAULT INJECTION STATS ===");

    Serial.print("Normal:      ");
    Serial.println(chaos_normal);

    Serial.print("CRC faults:  ");
    Serial.println(chaos_crc);

    Serial.print("Drops:       ");
    Serial.println(chaos_drop);

    Serial.print("Duplicates:  ");
    Serial.println(chaos_duplicate);

    Serial.print("Delays:      ");
    Serial.println(chaos_delay);

    uint32_t total_attempts =
        chaos_normal +
        chaos_crc +
        chaos_drop +
        chaos_duplicate +
        chaos_delay;

    Serial.print("TX attempts: ");
    Serial.println(total_attempts);

    Serial.println("=============================");
}


bool WaitForAck(
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


void SendSerumPing(
    uint16_t sequence,
    FaultMode fault
)
{
    SerumPacket ping = {0};

    ping.version =
        SERUM_VERSION;

    ping.type =
        SERUM_MSG_PING;

    ping.length = 0;

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

    if (tx_length == 0)
    {
        Serial.println(
            "Packet encoding failed"
        );

        return;
    }

    if (
        fault ==
        FAULT_BAD_CRC
    )
    {
        tx_buffer[
            tx_length - 1
        ] ^= 0x01;

        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(
            " sent [BAD CRC]"
        );

        SerumUART.write(
            tx_buffer,
            tx_length
        );

        return;
    }

    if (
        fault ==
        FAULT_DROP
    )
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(
            " [DROPPED]"
        );

        return;
    }

    if (
        fault ==
        FAULT_DELAY
    )
    {
        Serial.print("PING #");
        Serial.print(sequence);
        Serial.println(
            " [DELAYED]"
        );

        delay(2000);
    }

    SerumUART.write(
        tx_buffer,
        tx_length
    );

    Serial.print("PING #");
    Serial.print(sequence);

    if (
        fault ==
        FAULT_DUPLICATE
    )
    {
        Serial.println(
            " sent [DUPLICATE]"
        );

        delay(50);

        SerumUART.write(
            tx_buffer,
            tx_length
        );
    }
    else
    {
        Serial.println(
            " sent [VALID]"
        );
    }
}


void SendStatsRequest(
    uint16_t sequence
)
{
    SerumPacket packet = {0};

    packet.version =
        SERUM_VERSION;

    packet.type =
        SERUM_MSG_COMMAND;

    packet.length = 1;

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

    if (tx_length > 0)
    {
        SerumUART.write(
            tx_buffer,
            tx_length
        );

        Serial.println();
        Serial.println(
            "Requesting STM32 statistics..."
        );
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


bool SendReliablePacket(
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

    if (tx_length == 0)
    {
        Serial.println(
            "Packet encoding failed"
        );

        failed_deliveries++;

        return false;
    }

    for (
        uint8_t attempt = 1;
        attempt <= max_attempts;
        attempt++
    )
    {
        if (attempt > 1)
        {
            total_retries++;
        }

        Serial.print("TX #");
        Serial.print(
            packet->sequence
        );
        Serial.print(" attempt ");
        Serial.println(attempt);

        uint32_t start_us =
            micros();

        SerumUART.write(
            tx_buffer,
            tx_length
        );

        if (
            WaitForAck(
                packet->sequence,
                timeout_ms
            )
        )
        {
            uint32_t rtt_us =
                micros() - start_us;

            successful_deliveries++;

            rtt_total_us +=
                rtt_us;

            if (
                rtt_us <
                rtt_min_us
            )
            {
                rtt_min_us =
                    rtt_us;
            }

            if (
                rtt_us >
                rtt_max_us
            )
            {
                rtt_max_us =
                    rtt_us;
            }

            Serial.print("ACK #");
            Serial.print(
                packet->sequence
            );
            Serial.println(
                " received"
            );

            Serial.print("RTT: ");
            Serial.print(rtt_us);
            Serial.println(" us");

            Serial.print(
                "Delivered after "
            );
            Serial.print(attempt);
            Serial.println(
                " attempt(s)"
            );

            return true;
        }

        total_timeouts++;

        Serial.println(
            "ACK timeout"
        );
    }

    failed_deliveries++;

    Serial.print("Packet #");
    Serial.print(
        packet->sequence
    );
    Serial.println(
        " delivery failed"
    );

    return false;
}


void RunReliablePing(void)
{
    SerumPacket packet = {0};

    packet.version =
        SERUM_VERSION;

    packet.type =
        SERUM_MSG_PING;

    packet.length = 0;

    packet.sequence =
        next_sequence++;

    bool success =
        SendReliablePacket(
            &packet,
            retry_max_attempts,
            retry_timeout_ms
        );

    if (success)
    {
        Serial.println("PASS");
    }
    else
    {
        Serial.println("FAIL");
    }
}


void RunBenchmark(void)
{
    const uint16_t packet_count =
        20;

    ResetLinkStats();

    Serial.println();
    Serial.println(
        "=== SERUM BENCHMARK ==="
    );

    Serial.print("Packets: ");
    Serial.println(packet_count);

    Serial.println();

    for (
        uint16_t i = 0;
        i < packet_count;
        i++
    )
    {
        SerumPacket packet = {0};

        packet.version =
            SERUM_VERSION;

        packet.type =
            SERUM_MSG_PING;

        packet.length = 0;

        packet.sequence =
            next_sequence++;

        SendReliablePacket(
            &packet,
            retry_max_attempts,
            retry_timeout_ms
        );

        delay(10);
    }

    Serial.println();
    Serial.println(
        "Benchmark complete."
    );

    PrintLinkStats();
}


FaultMode GetRandomChaosFault(void)
{
    long roll =
        random(0, 100);

    if (roll < 70)
    {
        return FAULT_NONE;
    }

    if (roll < 80)
    {
        return FAULT_BAD_CRC;
    }

    if (roll < 90)
    {
        return FAULT_DROP;
    }

    if (roll < 95)
    {
        return FAULT_DUPLICATE;
    }

    return FAULT_DELAY;
}


const char *FaultModeName(
    FaultMode fault
)
{
    if (fault == FAULT_NONE)
    {
        return "NORMAL";
    }

    if (
        fault ==
        FAULT_BAD_CRC
    )
    {
        return "BAD CRC";
    }

    if (
        fault ==
        FAULT_DROP
    )
    {
        return "DROP";
    }

    if (
        fault ==
        FAULT_DUPLICATE
    )
    {
        return "DUPLICATE";
    }

    if (
        fault ==
        FAULT_DELAY
    )
    {
        return "DELAY";
    }

    return "UNKNOWN";
}


void SendWithFault(
    const uint8_t *buffer,
    uint16_t length,
    FaultMode fault
)
{
    if (length == 0)
    {
        return;
    }

    if (
        fault ==
        FAULT_DROP
    )
    {
        chaos_drop++;

        return;
    }

    if (
        fault ==
        FAULT_BAD_CRC
    )
    {
        chaos_crc++;

        uint8_t corrupted[
            SERUM_HEADER_SIZE +
            SERUM_MAX_PAYLOAD_SIZE +
            SERUM_CRC_SIZE
        ];

        memcpy(
            corrupted,
            buffer,
            length
        );

        corrupted[
            length - 1
        ] ^= 0x01;

        SerumUART.write(
            corrupted,
            length
        );

        return;
    }

    if (
        fault ==
        FAULT_DUPLICATE
    )
    {
        chaos_duplicate++;

        SerumUART.write(
            buffer,
            length
        );

        delay(5);

        SerumUART.write(
            buffer,
            length
        );

        return;
    }

    if (
        fault ==
        FAULT_DELAY
    )
    {
        chaos_delay++;

        delay(
            random(50, 200)
        );

        SerumUART.write(
            buffer,
            length
        );

        return;
    }

    chaos_normal++;

    SerumUART.write(
        buffer,
        length
    );
}


bool SendReliableChaosPacket(
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

    if (tx_length == 0)
    {
        failed_deliveries++;

        return false;
    }

    for (
        uint8_t attempt = 1;
        attempt <= max_attempts;
        attempt++
    )
    {
        if (attempt > 1)
        {
            total_retries++;
        }

        FaultMode fault =
            GetRandomChaosFault();

        Serial.print("TX #");
        Serial.print(
            packet->sequence
        );

        Serial.print(" attempt ");
        Serial.print(attempt);

        Serial.print(" [");
        Serial.print(
            FaultModeName(
                fault
            )
        );
        Serial.println("]");

        uint32_t start_us =
            micros();

        SendWithFault(
            tx_buffer,
            tx_length,
            fault
        );

        if (
            WaitForAck(
                packet->sequence,
                timeout_ms
            )
        )
        {
            uint32_t rtt_us =
                micros() - start_us;

            successful_deliveries++;

            rtt_total_us +=
                rtt_us;

            if (
                rtt_us <
                rtt_min_us
            )
            {
                rtt_min_us =
                    rtt_us;
            }

            if (
                rtt_us >
                rtt_max_us
            )
            {
                rtt_max_us =
                    rtt_us;
            }

            Serial.print("ACK #");
            Serial.print(
                packet->sequence
            );
            Serial.println(
                " received"
            );

            return true;
        }

        total_timeouts++;

        Serial.println(
            "ACK timeout"
        );
    }

    failed_deliveries++;

    Serial.print("Packet #");
    Serial.print(
        packet->sequence
    );
    Serial.println(
        " failed"
    );

    return false;
}


void RunChaosTest(void)
{
    const uint16_t packet_count =
        100;

    ResetLinkStats();
    ResetChaosStats();

    Serial.println();
    Serial.println(
        "=== GLITCH GOBLIN CHAOS TEST ==="
    );

    Serial.print(
        "Transactions: "
    );
    Serial.println(
        packet_count
    );

    Serial.println(
        "Fault probabilities:"
    );

    Serial.println(
        "Normal:    70%"
    );

    Serial.println(
        "Bad CRC:   10%"
    );

    Serial.println(
        "Drop:      10%"
    );

    Serial.println(
        "Duplicate: 5%"
    );

    Serial.println(
        "Delay:     5%"
    );

    Serial.println();

    for (
        uint16_t i = 0;
        i < packet_count;
        i++
    )
    {
        SerumPacket packet = {0};

        packet.version =
            SERUM_VERSION;

        packet.type =
            SERUM_MSG_PING;

        packet.length = 0;

        packet.sequence =
            next_sequence++;

        SendReliableChaosPacket(
            &packet,
            retry_max_attempts,
            retry_timeout_ms
        );

        delay(10);
    }

    Serial.println();
    Serial.println(
        "Chaos test complete."
    );

    PrintChaosStats();
    PrintLinkStats();
}


void PrintHelp(void)
{
    Serial.println();
    Serial.println("Commands:");
    Serial.println("normal");
    Serial.println("crc");
    Serial.println("drop");
    Serial.println("duplicate");
    Serial.println("delay");
    Serial.println("stats");
    Serial.println("reliable");
    Serial.println("bench");
    Serial.println("chaos");
    Serial.println("help");
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

    randomSeed(
        micros()
    );

    delay(1000);

    Serial.println();
    Serial.println(
        "=== GLITCH GOBLIN ==="
    );

    PrintHelp();

    Serial.println();
}


void loop()
{
    if (!Serial.available())
    {
        return;
    }

    String command =
        Serial.readStringUntil(
            '\n'
        );

    command.trim();
    command.toLowerCase();

    if (
        command ==
        "normal"
    )
    {
        RunFaultTest(
            FAULT_NONE,
            true
        );
    }

    else if (
        command ==
        "crc"
    )
    {
        RunFaultTest(
            FAULT_BAD_CRC,
            false
        );
    }

    else if (
        command ==
        "drop"
    )
    {
        RunFaultTest(
            FAULT_DROP,
            false
        );
    }

    else if (
        command ==
        "duplicate"
    )
    {
        RunFaultTest(
            FAULT_DUPLICATE,
            true
        );
    }

    else if (
        command ==
        "delay"
    )
    {
        RunFaultTest(
            FAULT_DELAY,
            true
        );
    }

    else if (
        command ==
        "stats"
    )
    {
        uint16_t sequence =
            next_sequence++;

        SendStatsRequest(
            sequence
        );

        if (
            !WaitForAck(
                sequence,
                500
            )
        )
        {
            Serial.println(
                "Stats ACK failed"
            );

            return;
        }

        if (
            !WaitForTelemetry(
                sequence,
                1000
            )
        )
        {
            Serial.println(
                "Telemetry timeout"
            );
        }
    }

    else if (
        command ==
        "reliable"
    )
    {
        RunReliablePing();
    }

    else if (
        command ==
        "bench"
    )
    {
        RunBenchmark();
    }

    else if (
        command ==
        "chaos"
    )
    {
        RunChaosTest();
    }

    else if (
        command ==
        "help"
    )
    {
        PrintHelp();
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