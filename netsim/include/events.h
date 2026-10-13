#ifndef NETSIM_EVENTS_H
#define NETSIM_EVENTS_H

#include "heap.h"

typedef enum {
    EV_NONE = 0,
    EV_FLOW_SEND,
    EV_PKT_ARRIVE,
    EV_TX_DONE,
    EV_LINK_FAIL,
    EV_LINK_RESTORE,
    EV_NODE_FAIL,
    EV_NODE_RESTORE,
    EV_WEIGHT_CHANGE,
    EV_TCP_TIMEOUT,
    EV_SAMPLE,
    EV_END
} EventType;

typedef struct SimEvent {
    double time;
    EventType type;
    int a;
    int b;
    int c;
    double value;
    int used;
    int gen;
} SimEvent;

typedef struct EventSched {
    Heap heap;
    SimEvent items[NETSIM_MAX_EVENTS];
    int free_ids[NETSIM_MAX_EVENTS];
    int free_count;
    int used_count;
} EventSched;

/* Create the event min-heap. Returns 0 on success. */
int events_init(EventSched *es);

/* Free heap storage. */
void events_free(EventSched *es);

/* Cancel all pending events. */
void events_clear(EventSched *es);

/* Push a timestamped event. Returns event index or -1. */
int events_schedule(EventSched *es, double time, EventType type, int a, int b, int c,
                    double value);

/* Pop the next event if it exists. */
int events_pop(EventSched *es, SimEvent *out);

/* Peek at the next event time. Returns 1 if an event exists. */
int events_peek_time(const EventSched *es, double *time);

#endif /* NETSIM_EVENTS_H */
