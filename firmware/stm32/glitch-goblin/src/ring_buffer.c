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
    if (next_head == rb->tail)  //if next head is tail then collisioon so buffer is full and we drop the byte
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
 
    if (rb->head == rb->tail)  // empty buffer
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