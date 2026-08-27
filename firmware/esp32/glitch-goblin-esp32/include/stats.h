#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#include "fault_engine.h"

void Stats_ResetLink(void);

void Stats_ResetChaos(void);

void Stats_RecordSuccess(
    uint32_t rtt_us
);

void Stats_RecordFailure(void);

void Stats_RecordRetry(void);

void Stats_RecordTimeout(void);

void Stats_RecordFault(
    FaultMode fault
);

void Stats_PrintLink(void);

void Stats_PrintChaos(void);

#endif