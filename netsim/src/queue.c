#include "queue.h"

int queue_init(PacketQueue *q, int cap)
{
    if (!q || cap <= 0) {
        return -1;
    }
    q->slots = (int *)calloc((size_t)cap, sizeof(int));
    if (!q->slots) {
        return -1;
    }
    q->cap = cap;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    return 0;
}

void queue_free(PacketQueue *q)
{
    if (!q) {
        return;
    }
    free(q->slots);
    q->slots = NULL;
    q->cap = 0;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
}

int queue_resize(PacketQueue *q, int cap)
{
    PacketQueue fresh;
    int i;
    int n;
    if (!q || cap <= 0) {
        return -1;
    }
    if (queue_init(&fresh, cap) != 0) {
        return -1;
    }
    n = q->count < cap ? q->count : cap;
    for (i = 0; i < n; i++) {
        fresh.slots[i] = q->slots[(q->head + i) % q->cap];
    }
    fresh.count = n;
    fresh.tail = n % cap;
    fresh.head = 0;
    queue_free(q);
    *q = fresh;
    return 0;
}

int queue_push(PacketQueue *q, int pkt_id)
{
    if (!q || !q->slots || q->count >= q->cap) {
        return -1;
    }
    q->slots[q->tail] = pkt_id;
    q->tail = (q->tail + 1) % q->cap;
    q->count += 1;
    return 0;
}

int queue_pop(PacketQueue *q, int *pkt_id)
{
    if (!q || !pkt_id || q->count <= 0) {
        return -1;
    }
    *pkt_id = q->slots[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count -= 1;
    return 0;
}

int queue_full(const PacketQueue *q)
{
    return q && q->count >= q->cap;
}

int queue_len(const PacketQueue *q)
{
    return q ? q->count : 0;
}
