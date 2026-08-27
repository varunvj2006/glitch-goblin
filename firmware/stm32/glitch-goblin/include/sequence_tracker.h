#ifndef SEQUENCE_TRACKER_H
#define SEQUENCE_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

#define SEQUENCE_HISTORY_SIZE 8

void SequenceTracker_Init(void);

bool SequenceTracker_HasSeen(
    uint16_t sequence
);

void SequenceTracker_Remember(
    uint16_t sequence
);

#endif