#ifndef NETSIM_QUEUE_H
#define NETSIM_QUEUE_H

#include "common.h"

typedef struct PacketQueue {
    int *slots;
    int cap;
    int count;
    int head;
    int tail;
} PacketQueue;

/* Allocate a FIFO of capacity cap. Returns 0 on success. */
int queue_init(PacketQueue *q, int cap);

/* Free slot storage. */
void queue_free(PacketQueue *q);

/* Rebuild the buffer with a new capacity, dropping overflow from the tail. */
int queue_resize(PacketQueue *q, int cap);

/* Enqueue a packet id. Returns 0, or -1 if full (tail drop). */
int queue_push(PacketQueue *q, int pkt_id);

/* Dequeue the oldest packet id. Returns 0 on success. */
int queue_pop(PacketQueue *q, int *pkt_id);

/* Return 1 if the buffer is at capacity. */
int queue_full(const PacketQueue *q);

/* Number of queued packets. */
int queue_len(const PacketQueue *q);

#endif /* NETSIM_QUEUE_H */
