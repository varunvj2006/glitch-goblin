#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE 128

typedef struct
{
    uint8_t data[RING_BUFFER_SIZE];

    volatile uint16_t head;
    volatile uint16_t tail;

    volatile uint32_t dropped_bytes;

} RingBuffer;

void RingBuffer_Init(RingBuffer *rb);

bool RingBuffer_Push(
    RingBuffer *rb,
    uint8_t byte
);

bool RingBuffer_Pop(
    RingBuffer *rb,
    uint8_t *byte
);

uint16_t RingBuffer_Available(
    const RingBuffer *rb
);

#endif