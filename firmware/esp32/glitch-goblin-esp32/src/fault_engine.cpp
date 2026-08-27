#include <Arduino.h>
#include <string.h>

#include "fault_engine.h"
#include "serum.h"
#include "serum_link.h"

void FaultEngine_Init(void)
{
    randomSeed(
        micros()
    );
}

FaultMode FaultEngine_GetRandomFault(void)
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

const char *FaultEngine_GetName(
    FaultMode fault
)
{
    if (fault == FAULT_NONE)
    {
        return "NORMAL";
    }

    if (fault == FAULT_BAD_CRC)
    {
        return "BAD CRC";
    }

    if (fault == FAULT_DROP)
    {
        return "DROP";
    }

    if (fault == FAULT_DUPLICATE)
    {
        return "DUPLICATE";
    }

    if (fault == FAULT_DELAY)
    {
        return "DELAY";
    }

    return "UNKNOWN";
}

void FaultEngine_SendWithFault(
    const uint8_t *buffer,
    uint16_t length,
    FaultMode fault
)
{
    if (length == 0)
    {
        return;
    }

    if (fault == FAULT_DROP)
    {
        return;
    }

    if (fault == FAULT_BAD_CRC)
    {
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

        SerumLink_WriteRaw(
            corrupted,
            length
        );

        return;
    }

    if (fault == FAULT_DUPLICATE)
    {
        SerumLink_WriteRaw(
            buffer,
            length
        );

        delay(5);

        SerumLink_WriteRaw(
            buffer,
            length
        );

        return;
    }

    if (fault == FAULT_DELAY)
    {
        delay(
            random(50, 200)
        );

        SerumLink_WriteRaw(
            buffer,
            length
        );

        return;
    }

    SerumLink_WriteRaw(
        buffer,
        length
    );
}