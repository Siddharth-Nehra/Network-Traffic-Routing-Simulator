#ifndef NETSIM_COMMON_H
#define NETSIM_COMMON_H

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define NETSIM_MAX_NODES 128
#define NETSIM_MAX_LINKS 512
#define NETSIM_MAX_NAME 32
#define NETSIM_MAX_PATH NETSIM_MAX_NODES
#define NETSIM_MAX_PACKETS 8192
#define NETSIM_MAX_EVENTS 16384
#define NETSIM_MAX_FLOWS 64
#define NETSIM_CHART 180
#define NETSIM_MAX_FLASH 48
#define NETSIM_INF 1.0e15
#define NETSIM_EPS 1.0e-12
#define NETSIM_NODE_RADIUS 18.0f
#define NETSIM_DEFAULT_BW_KBPS 1000.0
#define NETSIM_DEFAULT_DELAY_MS 10.0
#define NETSIM_DEFAULT_LOSS 0.0
#define NETSIM_DEFAULT_WEIGHT 1.0
#define NETSIM_DEFAULT_QUEUE_K 50
#define NETSIM_DEFAULT_PKT 500

typedef enum {
    NODE_UP = 0,
    NODE_DOWN = 1
} NodeState;

typedef enum {
    LINK_UP = 0,
    LINK_DOWN = 1
} LinkState;

typedef enum {
    PROTO_UDP = 0,
    PROTO_TCP = 1
} ProtocolKind;

typedef enum {
    FLOW_CBR = 0,
    FLOW_BURSTY = 1,
    FLOW_TCP = 2,
    FLOW_POISSON = 3
} FlowType;

typedef enum {
    ROUTING_DIJKSTRA = 0,
    ROUTING_BELLMAN_FORD = 1
} RoutingAlgo;

typedef enum {
    COST_STATIC_WEIGHT = 0,
    COST_HOP_COUNT = 1,
    COST_DYNAMIC = 2
} CostMode;

/* Return current monotonic time in microseconds. */
static inline long netsim_now_us(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (long)((counter.QuadPart * 1000000LL) / freq.QuadPart);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (long)ts.tv_sec * 1000000L + (long)(ts.tv_nsec / 1000L);
#endif
}

/* True if value is a finite routing cost (not infinity). */
static inline int netsim_finite(double x)
{
    return x < NETSIM_INF * 0.5;
}

#endif /* NETSIM_COMMON_H */
