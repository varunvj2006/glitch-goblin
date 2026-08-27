#include <Arduino.h>

#include "config.h"

static uint8_t retry_attempts = 3;
static uint32_t ack_timeout_ms = 500;
static uint16_t benchmark_packets = 20;
static uint16_t chaos_packets = 100;
static uint8_t normal_rate = 70;
static uint8_t crc_rate = 10;
static uint8_t drop_rate = 10;
static uint8_t duplicate_rate = 5;
static uint8_t delay_rate = 5;

void Config_Init(void)
{
}

uint8_t Config_GetNormalRate(void)
{
    return normal_rate;
}

uint8_t Config_GetCrcRate(void)
{
    return crc_rate;
}

uint8_t Config_GetDropRate(void)
{
    return drop_rate;
}

uint8_t Config_GetDuplicateRate(void)
{
    return duplicate_rate;
}

uint8_t Config_GetDelayRate(void)
{
    return delay_rate;
}

bool Config_SetFaultRates(
    uint8_t normal,
    uint8_t crc,
    uint8_t drop,
    uint8_t duplicate,
    uint8_t delay
)
{
    uint16_t total =
        normal +
        crc +
        drop +
        duplicate +
        delay;

    if (total != 100)
    {
        return false;
    }

    normal_rate =
        normal;

    crc_rate =
        crc;

    drop_rate =
        drop;

    duplicate_rate =
        duplicate;

    delay_rate =
        delay;

    return true;
}
uint8_t Config_GetRetryAttempts(void)
{
    return retry_attempts;
}

uint32_t Config_GetAckTimeoutMs(void)
{
    return ack_timeout_ms;
}

uint16_t Config_GetBenchmarkPackets(void)
{
    return benchmark_packets;
}

uint16_t Config_GetChaosPackets(void)
{
    return chaos_packets;
}

bool Config_SetRetryAttempts(
    uint8_t attempts
)
{
    if (
        attempts < 1 ||
        attempts > 10
    )
    {
        return false;
    }

    retry_attempts =
        attempts;

    return true;
}

bool Config_SetAckTimeoutMs(
    uint32_t timeout_ms
)
{
    if (
        timeout_ms < 50 ||
        timeout_ms > 5000
    )
    {
        return false;
    }

    ack_timeout_ms =
        timeout_ms;

    return true;
}

bool Config_SetBenchmarkPackets(
    uint16_t packets
)
{
    if (
        packets < 1 ||
        packets > 10000
    )
    {
        return false;
    }

    benchmark_packets =
        packets;

    return true;
}

bool Config_SetChaosPackets(
    uint16_t packets
)
{
    if (
        packets < 1 ||
        packets > 10000
    )
    {
        return false;
    }

    chaos_packets =
        packets;

    return true;
}

void Config_Print(void)
{
    Serial.println();

    Serial.println(
        "=== GLITCH GOBLIN CONFIG ==="
    );

    Serial.print(
        "Retry attempts:    "
    );
    Serial.println(
        retry_attempts
    );

    Serial.print(
        "ACK timeout:       "
    );
    Serial.print(
        ack_timeout_ms
    );
    Serial.println(
        " ms"
    );

    Serial.print(
        "Benchmark packets: "
    );
    Serial.println(
        benchmark_packets
    );

    Serial.print(
        "Chaos packets:     "
    );
    Serial.println(
        chaos_packets
    );

    Serial.println();

    Serial.println(
        "Fault probabilities:"
    );

    Serial.print(
        "Normal:            "
    );
    Serial.print(
        normal_rate
    );
    Serial.println("%");

    Serial.print(
        "CRC:               "
    );
    Serial.print(
        crc_rate
    );
    Serial.println("%");

    Serial.print(
        "Drop:              "
    );
    Serial.print(
        drop_rate
    );
    Serial.println("%");

    Serial.print(
        "Duplicate:         "
    );
    Serial.print(
        duplicate_rate
    );
    Serial.println("%");

    Serial.print(
        "Delay:             "
    );
    Serial.print(
        delay_rate
    );
    Serial.println("%");
    Serial.println(
        "============================"
    );
}