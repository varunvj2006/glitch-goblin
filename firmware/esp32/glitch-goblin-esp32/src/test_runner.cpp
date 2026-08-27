#include <Arduino.h>

#include "test_runner.h"
#include "serum.h"
#include "serum_link.h"
#include "stats.h"

void TestRunner_RunFault(
    FaultMode fault,
    bool expect_ack
)
{
    uint16_t sequence =
        SerumLink_NextSequence();

    SerumLink_SendPing(
        sequence,
        fault
    );

    bool ack =
        SerumLink_WaitForAck(
            sequence,
            500
        );

    bool passed =
        (
            ack ==
            expect_ack
        );

    if (passed)
    {
        Serial.println(
            "PASS"
        );
    }
    else
    {
        Serial.println(
            "FAIL"
        );
    }
}

void TestRunner_RunStats(void)
{
    uint16_t sequence =
        SerumLink_NextSequence();

    SerumLink_SendStatsRequest(
        sequence
    );

    if (
        !SerumLink_WaitForAck(
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
        !SerumLink_WaitForTelemetry(
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

void TestRunner_RunReliablePing(void)
{
    SerumPacket packet = {0};

    packet.version =
        SERUM_VERSION;

    packet.type =
        SERUM_MSG_PING;

    packet.length =
        0;

    packet.sequence =
        SerumLink_NextSequence();

    bool success =
        SerumLink_SendReliablePacket(
            &packet,
            SerumLink_GetRetryMaxAttempts(),
            SerumLink_GetRetryTimeoutMs()
        );

    if (success)
    {
        Serial.println(
            "PASS"
        );
    }
    else
    {
        Serial.println(
            "FAIL"
        );
    }
}

void TestRunner_RunBenchmark(void)
{
    const uint16_t packet_count =
        20;

    Stats_ResetLink();

    Serial.println();

    Serial.println(
        "=== SERUM BENCHMARK ==="
    );

    Serial.print(
        "Packets: "
    );

    Serial.println(
        packet_count
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

        packet.length =
            0;

        packet.sequence =
            SerumLink_NextSequence();

        SerumLink_SendReliablePacket(
            &packet,
            SerumLink_GetRetryMaxAttempts(),
            SerumLink_GetRetryTimeoutMs()
        );

        delay(10);
    }

    Serial.println();

    Serial.println(
        "Benchmark complete."
    );

    Stats_PrintLink();
}

void TestRunner_RunChaos(void)
{
    const uint16_t packet_count =
        100;

    Stats_ResetLink();
    Stats_ResetChaos();

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

        packet.length =
            0;

        packet.sequence =
            SerumLink_NextSequence();

        SerumLink_SendReliableChaosPacket(
            &packet,
            SerumLink_GetRetryMaxAttempts(),
            SerumLink_GetRetryTimeoutMs()
        );

        delay(10);
    }

    Serial.println();

    Serial.println(
        "Chaos test complete."
    );

    Stats_PrintChaos();

    Stats_PrintLink();
}