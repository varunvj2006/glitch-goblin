#include "sequence_tracker.h"

static uint16_t sequence_history[
    SEQUENCE_HISTORY_SIZE
];

static uint8_t sequence_count = 0;
static uint8_t sequence_index = 0;


void SequenceTracker_Init(void)
{
    sequence_count = 0;
    sequence_index = 0;
}


bool SequenceTracker_HasSeen(
    uint16_t sequence
)
{
    for (
        uint8_t i = 0;
        i < sequence_count;
        i++
    )
    {
        if (
            sequence_history[i] ==
            sequence
        )
        {
            return true;
        }
    }

    return false;
}


void SequenceTracker_Remember(
    uint16_t sequence
)
{
    sequence_history[
        sequence_index
    ] = sequence;

    sequence_index++;

    if (
        sequence_index >=
        SEQUENCE_HISTORY_SIZE
    )
    {
        sequence_index = 0;
    }

    if (
        sequence_count <
        SEQUENCE_HISTORY_SIZE
    )
    {
        sequence_count++;
    }
}