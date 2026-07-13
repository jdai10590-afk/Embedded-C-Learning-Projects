#include "ring_buffer.h"
#include "debug.h"

void ring_buffer_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    DEBUG_PRINT("ring buffer init done\n");
}

int ring_buffer_is_empty(const RingBuffer *rb)
{
    return rb->count == 0;
}

int ring_buffer_is_full(const RingBuffer *rb)
{
    return rb->count == RING_BUFFER_SIZE;
}

int ring_buffer_push(RingBuffer *rb, char byte)
{
    if(ring_buffer_is_full(rb))
    {
        DEBUG_PRINT("ring buffer full, push failed\n");
        return 0;
    }

    rb->data[rb->head] = byte;

    rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
    rb->count++;

    return 1;
}

int ring_buffer_pop(RingBuffer *rb, char *byte)
{
    if(ring_buffer_is_empty(rb))
    {
        DEBUG_PRINT("ring buffer empty, pop failed\n");
        return 0;
    }

    *byte = rb->data[rb->tail];

    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    rb->count--;

    return 1;
}