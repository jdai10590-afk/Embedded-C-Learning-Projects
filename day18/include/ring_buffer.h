#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFFER_SIZE 64

typedef struct
{
    char data[RING_BUFFER_SIZE];

    unsigned int head;
    unsigned int tail;
    unsigned int count;
} RingBuffer;

void ring_buffer_init(RingBuffer *rb);

int ring_buffer_push(RingBuffer *rb, char byte);
int ring_buffer_pop(RingBuffer *rb, char *byte);

int ring_buffer_is_empty(const RingBuffer *rb);
int ring_buffer_is_full(const RingBuffer *rb);

#endif