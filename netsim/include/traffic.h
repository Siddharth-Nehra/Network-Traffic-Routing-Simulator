#ifndef NETSIM_TRAFFIC_H
#define NETSIM_TRAFFIC_H

#include "common.h"
#include "routing.h"

typedef struct Flow {
    int id;
    int used;
    int src;
    int dst;
    FlowType type;
    ProtocolKind proto;
    double rate_kbps;
    int packet_size;
    double start_time;
    double stop_time;
    double on_ms;
    double off_ms;
    int on_state;
    double state_until;
    int seq;
    int active;
    double cwnd;
    double ssthresh;
    int in_flight;
    int snd_una;
    int snd_next;
    int rcv_next;
    int dup_acks;
    double srtt;
    double rto;
    int timeout_gen;
    int sent;
    int recv;
    int dropped;
    int acked;
    double lat_sum;
    double lat_min;
    double lat_max;
    int lat_n;
    double last_lat;
    double jitter_sum;
    double bytes_recv;
    double bytes_sent;
    double first_recv;
    double last_recv;
    int last_path[NETSIM_MAX_PATH];
    int last_path_len;
    double last_path_cost;
} Flow;

struct Simulator;

/* Create a flow and schedule its first send. Returns flow id or -1. */
int traffic_add_flow(struct Simulator *sim, int src, int dst, FlowType type,
                     double rate_kbps, int packet_size, double start_t, double stop_t);

/* Inter-packet gap in seconds for a CBR/Poisson/ON flow. */
double traffic_interval(const Flow *f);

/* Handle EV_FLOW_SEND: maybe emit a packet and schedule the next send. */
void traffic_on_send(struct Simulator *sim, int flow_id);

#endif /* NETSIM_TRAFFIC_H */
