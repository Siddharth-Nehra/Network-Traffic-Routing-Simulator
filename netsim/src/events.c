#include "events.h"

int events_init(EventSched *es)
{
    int i;
    if (!es) {
        return -1;
    }
    memset(es, 0, sizeof(*es));
    if (heap_init(&es->heap) != 0) {
        return -1;
    }
    es->free_count = NETSIM_MAX_EVENTS;
    for (i = 0; i < NETSIM_MAX_EVENTS; i++) {
        es->items[i].used = 0;
        es->free_ids[i] = NETSIM_MAX_EVENTS - 1 - i;
    }
    return 0;
}

void events_free(EventSched *es)
{
    if (!es) {
        return;
    }
    heap_free(&es->heap);
    memset(es, 0, sizeof(*es));
}

void events_clear(EventSched *es)
{
    int i;
    if (!es) {
        return;
    }
    heap_clear(&es->heap);
    es->free_count = NETSIM_MAX_EVENTS;
    for (i = 0; i < NETSIM_MAX_EVENTS; i++) {
        es->items[i].used = 0;
        es->free_ids[i] = NETSIM_MAX_EVENTS - 1 - i;
    }
    es->used_count = 0;
}

static int events_alloc_slot(EventSched *es)
{
    int idx;
    if (es->free_count <= 0) {
        return -1;
    }
    es->free_count -= 1;
    idx = es->free_ids[es->free_count];
    return idx;
}

int events_schedule(EventSched *es, double time, EventType type, int a, int b, int c,
                    double value)
{
    int idx;
    SimEvent *ev;
    if (!es) {
        return -1;
    }
    idx = events_alloc_slot(es);
    if (idx < 0) {
        return -1;
    }
    ev = &es->items[idx];
    ev->time = time;
    ev->type = type;
    ev->a = a;
    ev->b = b;
    ev->c = c;
    ev->value = value;
    ev->used = 1;
    ev->gen += 1;
    if (heap_push(&es->heap, time, idx, ev) != 0) {
        ev->used = 0;
        es->free_ids[es->free_count] = idx;
        es->free_count += 1;
        return -1;
    }
    es->used_count += 1;
    return idx;
}

int events_pop(EventSched *es, SimEvent *out)
{
    HeapItem item;
    SimEvent *ev;
    if (!es || !out) {
        return -1;
    }
    if (heap_pop(&es->heap, &item) != 0) {
        return -1;
    }
    ev = &es->items[item.value];
    *out = *ev;
    ev->used = 0;
    es->free_ids[es->free_count] = item.value;
    es->free_count += 1;
    es->used_count -= 1;
    return 0;
}

int events_peek_time(const EventSched *es, double *time)
{
    HeapItem item;
    if (!es || !time || heap_peek(&es->heap, &item) != 0) {
        return 0;
    }
    *time = item.key;
    return 1;
}
