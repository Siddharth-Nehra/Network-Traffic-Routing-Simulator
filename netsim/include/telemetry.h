#ifndef NETSIM_TELEMETRY_H
#define NETSIM_TELEMETRY_H

#include "common.h"

typedef struct Telemetry {
    int sent;
    int recv;
    int dropped;
    double lat_sum;
    double lat_min;
    double lat_max;
    int lat_n;
    double jitter_sum;
    double last_lat;
    double bytes_recv;
    double qlen_hist[NETSIM_CHART];
    double lat_hist[NETSIM_CHART];
    double thr_hist[NETSIM_CHART];
    double cwnd_hist[NETSIM_CHART];
    double t_hist[NETSIM_CHART];
    int chart_n;
    int chart_head;
    double last_sample_t;
    double last_bytes;
    double fail_time;
    double reroute_time;
    int fail_link;
    int old_path[NETSIM_MAX_PATH];
    int old_path_len;
    int new_path[NETSIM_MAX_PATH];
    int new_path_len;
    int waiting_reroute;
    double mm1_measured_qdelay;
    double mm1_theory_qdelay;
} Telemetry;

struct Simulator;

/* Zero counters and charts. */
void telemetry_init(Telemetry *t);

/* Record a delivered packet's latency. */
void telemetry_on_deliver(struct Simulator *sim, int flow_id, double latency);

/* Record a drop. */
void telemetry_on_drop(struct Simulator *sim, int flow_id);

/* Snapshot rolling chart series at time t. */
void telemetry_sample(struct Simulator *sim);

/* Average / min / max latency in milliseconds. */
void telemetry_latency_ms(const Telemetry *t, double *avg, double *mn, double *mx);

/* Throughput in kbps over the whole run. */
double telemetry_throughput_kbps(const Telemetry *t, double sim_time);

/* Packet delivery ratio in percent. */
double telemetry_pdr(const Telemetry *t);

/* Print a summary table and M/M/1 check to stdout. */
void telemetry_print_summary(struct Simulator *sim, const char *scenario);

#endif /* NETSIM_TELEMETRY_H */
