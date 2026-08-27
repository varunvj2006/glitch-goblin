#ifndef SERUM_LINK_H
#define SERUM_LINK_H

#include <stdint.h>

#include "serum.h"
#include "fault_engine.h"

void SerumLink_Init(void);

void SerumLink_WriteRaw(
    const uint8_t *buffer,
    uint16_t length
);

uint16_t SerumLink_NextSequence(void);



bool SerumLink_WaitForAck(
    uint16_t sequence,
    uint32_t timeout_ms
);

bool SerumLink_WaitForTelemetry(
    uint16_t sequence,
    uint32_t timeout_ms
);

void SerumLink_SendPing(
    uint16_t sequence,
    FaultMode fault
);

void SerumLink_SendStatsRequest(
    uint16_t sequence
);

bool SerumLink_SendReliablePacket(
    const SerumPacket *packet,
    uint8_t max_attempts,
    uint32_t timeout_ms
);

bool SerumLink_SendReliableChaosPacket(
    const SerumPacket *packet,
    uint8_t max_attempts,
    uint32_t timeout_ms
);

#endif