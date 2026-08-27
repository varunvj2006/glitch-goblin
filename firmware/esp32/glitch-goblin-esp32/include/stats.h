#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#include "fault_engine.h"

void Stats_ResetLink(void);

void Stats_ResetChaos(void);

void Stats_RecordSuccess(
    uint32_t rtt_us,
    uint8_t attempt
);

void Stats_RecordFailure(void);

void Stats_RecordRetry(void);

void Stats_RecordTimeout(void);

void Stats_RecordFault(
    FaultMode fault
);

void Stats_PrintLink(void);

void Stats_PrintChaos(void);

float Stats_GetDeliveryRate(void);

float Stats_GetRecoveryRate(void);

uint32_t Stats_GetFirstAttemptSuccesses(void);

uint32_t Stats_GetRecoveredDeliveries(void);

uint32_t Stats_GetFailedDeliveries(void);

#endif