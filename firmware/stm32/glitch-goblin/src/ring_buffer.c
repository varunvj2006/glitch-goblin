#include "ring_buffer.h"

static uint16_t NextIndex(uint16_t index)
{
    return (index + 1) % RING_BUFFER_SIZE;
}


void RingBuffer_Init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->dropped_bytes = 0;
}


bool RingBuffer_Push(RingBuffer *rb, uint8_t byte)
{
    uint16_t next_head = NextIndex(rb->head);

    /*
     * If moving head forward would collide
     * with tail, the buffer is full.
     */
    if (next_head == rb->tail)
    {
        rb->dropped_bytes++;
        return false;
    }

    rb->data[rb->head] = byte;

    rb->head = next_head;

    return true;
}


bool RingBuffer_Pop(RingBuffer *rb, uint8_t *byte)
{
    /*
     * head == tail means buffer is empty.
     */
    if (rb->head == rb->tail)
    {
        return false;
    }

    *byte = rb->data[rb->tail];

    rb->tail = NextIndex(rb->tail);

    return true;
}


uint16_t RingBuffer_Available(const RingBuffer *rb)
{
    if (rb->head >= rb->tail)
    {
        return rb->head - rb->tail;
    }

    return RING_BUFFER_SIZE
         - rb->tail
         + rb->head;
}