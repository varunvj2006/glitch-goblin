#include <Arduino.h>

#include "stats.h"

static uint32_t rtt_min_us =
    UINT32_MAX;

static uint32_t rtt_max_us =
    0;

static uint64_t rtt_total_us =
    0;

static uint32_t successful_deliveries =
    0;

static uint32_t failed_deliveries =
    0;

static uint32_t first_attempt_successes =
    0;

static uint32_t recovered_deliveries =
    0;

static uint32_t total_retries =
    0;

static uint32_t total_timeouts =
    0;

static uint32_t chaos_normal =
    0;

static uint32_t chaos_crc =
    0;

static uint32_t chaos_drop =
    0;

static uint32_t chaos_duplicate =
    0;

static uint32_t chaos_delay =
    0;


void Stats_ResetLink(void)
{
    rtt_min_us =
        UINT32_MAX;

    rtt_max_us =
        0;

    rtt_total_us =
        0;

    successful_deliveries =
        0;

    failed_deliveries =
        0;

    first_attempt_successes =
        0;

    recovered_deliveries =
        0;

    total_retries =
        0;

    total_timeouts =
        0;
}


void Stats_ResetChaos(void)
{
    chaos_normal = 0;
    chaos_crc = 0;
    chaos_drop = 0;
    chaos_duplicate = 0;
    chaos_delay = 0;
}


void Stats_RecordSuccess(
    uint32_t rtt_us,
    uint8_t attempt
)
{
    successful_deliveries++;

    if (attempt == 1)
    {
        first_attempt_successes++;
    }
    else
    {
        recovered_deliveries++;
    }

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
}


void Stats_RecordFailure(void)
{
    failed_deliveries++;
}


void Stats_RecordRetry(void)
{
    total_retries++;
}


void Stats_RecordTimeout(void)
{
    total_timeouts++;
}


void Stats_RecordFault(
    FaultMode fault
)
{
    if (fault == FAULT_NONE)
    {
        chaos_normal++;
    }

    else if (
        fault ==
        FAULT_BAD_CRC
    )
    {
        chaos_crc++;
    }

    else if (
        fault ==
        FAULT_DROP
    )
    {
        chaos_drop++;
    }

    else if (
        fault ==
        FAULT_DUPLICATE
    )
    {
        chaos_duplicate++;
    }

    else if (
        fault ==
        FAULT_DELAY
    )
    {
        chaos_delay++;
    }
}


float Stats_GetDeliveryRate(void)
{
    uint32_t total_transactions =
        successful_deliveries +
        failed_deliveries;

    if (total_transactions == 0)
    {
        return 0.0f;
    }

    return
        (
            (float)successful_deliveries /
            (float)total_transactions
        ) *
        100.0f;
}


float Stats_GetRecoveryRate(void)
{
    uint32_t recovery_opportunities =
        recovered_deliveries +
        failed_deliveries;

    if (recovery_opportunities == 0)
    {
        return 100.0f;
    }

    return
        (
            (float)recovered_deliveries /
            (float)recovery_opportunities
        ) *
        100.0f;
}


uint32_t Stats_GetFirstAttemptSuccesses(void)
{
    return
        first_attempt_successes;
}


uint32_t Stats_GetRecoveredDeliveries(void)
{
    return
        recovered_deliveries;
}


uint32_t Stats_GetFailedDeliveries(void)
{
    return
        failed_deliveries;
}


void Stats_PrintLink(void)
{
    Serial.println();

    Serial.println(
        "=== SERUM LINK STATS ==="
    );

    Serial.print(
        "Successful:             "
    );
    Serial.println(
        successful_deliveries
    );

    Serial.print(
        "Failed:                 "
    );
    Serial.println(
        failed_deliveries
    );

    Serial.print(
        "First-attempt success:  "
    );
    Serial.println(
        first_attempt_successes
    );

    Serial.print(
        "Recovered by retry:     "
    );
    Serial.println(
        recovered_deliveries
    );

    Serial.print(
        "Retries:                "
    );
    Serial.println(
        total_retries
    );

    Serial.print(
        "Timeouts:               "
    );
    Serial.println(
        total_timeouts
    );

    Serial.print(
        "Delivery:               "
    );
    Serial.print(
        Stats_GetDeliveryRate(),
        2
    );
    Serial.println("%");

    Serial.print(
        "Recovery rate:          "
    );
    Serial.print(
        Stats_GetRecoveryRate(),
        2
    );
    Serial.println("%");

    if (
        successful_deliveries > 0
    )
    {
        uint32_t average_rtt =
            (uint32_t)(
                rtt_total_us /
                successful_deliveries
            );

        Serial.print(
            "Min RTT:               "
        );
        Serial.print(
            rtt_min_us
        );
        Serial.println(
            " us"
        );

        Serial.print(
            "Max RTT:               "
        );
        Serial.print(
            rtt_max_us
        );
        Serial.println(
            " us"
        );

        Serial.print(
            "Average RTT:           "
        );
        Serial.print(
            average_rtt
        );
        Serial.println(
            " us"
        );
    }

    Serial.println(
        "========================"
    );
}


void Stats_PrintChaos(void)
{
    Serial.println();

    Serial.println(
        "=== FAULT INJECTION STATS ==="
    );

    Serial.print(
        "Normal:      "
    );
    Serial.println(
        chaos_normal
    );

    Serial.print(
        "CRC faults:  "
    );
    Serial.println(
        chaos_crc
    );

    Serial.print(
        "Drops:       "
    );
    Serial.println(
        chaos_drop
    );

    Serial.print(
        "Duplicates:  "
    );
    Serial.println(
        chaos_duplicate
    );

    Serial.print(
        "Delays:      "
    );
    Serial.println(
        chaos_delay
    );

    uint32_t total_attempts =
        chaos_normal +
        chaos_crc +
        chaos_drop +
        chaos_duplicate +
        chaos_delay;

    Serial.print(
        "TX attempts: "
    );
    Serial.println(
        total_attempts
    );

    Serial.println(
        "============================="
    );
}