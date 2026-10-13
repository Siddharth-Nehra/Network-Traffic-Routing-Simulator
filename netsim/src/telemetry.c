#include "telemetry.h"
#include "sim.h"

void telemetry_init(Telemetry *t)
{
    if (!t) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->lat_min = 1e9;
    t->reroute_time = -1.0;
    t->fail_link = -1;
    t->mm1_theory_qdelay = -1.0;
    t->mm1_measured_qdelay = -1.0;
}

void telemetry_on_deliver(Simulator *sim, int flow_id, double latency)
{
    Telemetry *t = &sim->tel;
    Flow *f;
    t->recv += 1;
    t->bytes_recv += (flow_id >= 0 && flow_id < NETSIM_MAX_FLOWS)
                         ? (double)sim->flows[flow_id].packet_size
                         : 0.0;
    t->lat_sum += latency;
    t->lat_n += 1;
    if (latency < t->lat_min) {
        t->lat_min = latency;
    }
    if (latency > t->lat_max) {
        t->lat_max = latency;
    }
    if (t->lat_n > 1) {
        t->jitter_sum += fabs(latency - t->last_lat);
    }
    t->last_lat = latency;
    if (flow_id < 0 || flow_id >= NETSIM_MAX_FLOWS || !sim->flows[flow_id].used) {
        return;
    }
    f = &sim->flows[flow_id];
    f->recv += 1;
    f->bytes_recv += (double)f->packet_size;
    f->lat_sum += latency;
    f->lat_n += 1;
    if (latency < f->lat_min) {
        f->lat_min = latency;
    }
    if (latency > f->lat_max) {
        f->lat_max = latency;
    }
    if (f->lat_n > 1) {
        f->jitter_sum += fabs(latency - f->last_lat);
    }
    f->last_lat = latency;
    if (f->first_recv <= 0.0) {
        f->first_recv = sim->now;
    }
    f->last_recv = sim->now;
}

void telemetry_on_drop(Simulator *sim, int flow_id)
{
    sim->tel.dropped += 1;
    if (flow_id >= 0 && flow_id < NETSIM_MAX_FLOWS && sim->flows[flow_id].used) {
        sim->flows[flow_id].dropped += 1;
    }
}

void telemetry_sample(Simulator *sim)
{
    Telemetry *t = &sim->tel;
    int idx;
    double dt;
    double thr;
    double avg_q = 0.0;
    double cw = 0.0;
    int nq = 0;
    int i;
    dt = sim->now - t->last_sample_t;
    if (dt < 1e-9) {
        dt = 0.1;
    }
    thr = ((t->bytes_recv - t->last_bytes) * 8.0) / (dt * 1000.0);
    t->last_bytes = t->bytes_recv;
    t->last_sample_t = sim->now;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        if (sim->graph.link_used[i]) {
            avg_q += (double)queue_len(&sim->graph.links[i].queue);
            nq += 1;
        }
    }
    if (nq > 0) {
        avg_q /= (double)nq;
    }
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        if (sim->flows[i].used && sim->flows[i].proto == PROTO_TCP) {
            cw = sim->flows[i].cwnd;
            break;
        }
    }
    if (t->chart_n < NETSIM_CHART) {
        idx = t->chart_n;
        t->chart_n += 1;
    } else {
        idx = t->chart_head;
        t->chart_head = (t->chart_head + 1) % NETSIM_CHART;
    }
    t->t_hist[idx] = sim->now;
    t->lat_hist[idx] = (t->lat_n > 0) ? (t->lat_sum / (double)t->lat_n) * 1000.0 : 0.0;
    t->thr_hist[idx] = thr;
    t->qlen_hist[idx] = avg_q;
    t->cwnd_hist[idx] = cw;
}

void telemetry_latency_ms(const Telemetry *t, double *avg, double *mn, double *mx)
{
    if (!t || t->lat_n <= 0) {
        if (avg) {
            *avg = 0.0;
        }
        if (mn) {
            *mn = 0.0;
        }
        if (mx) {
            *mx = 0.0;
        }
        return;
    }
    if (avg) {
        *avg = (t->lat_sum / (double)t->lat_n) * 1000.0;
    }
    if (mn) {
        *mn = t->lat_min * 1000.0;
    }
    if (mx) {
        *mx = t->lat_max * 1000.0;
    }
}

double telemetry_throughput_kbps(const Telemetry *t, double sim_time)
{
    if (!t || sim_time <= 0.0) {
        return 0.0;
    }
    return (t->bytes_recv * 8.0) / (sim_time * 1000.0);
}

double telemetry_pdr(const Telemetry *t)
{
    if (!t || t->sent <= 0) {
        return 0.0;
    }
    return 100.0 * (double)t->recv / (double)t->sent;
}

/* Compare measured bottleneck queueing delay to M/M/1 theory. */
static void telemetry_mm1(Simulator *sim)
{
    int i;
    int best = -1;
    double best_util = -1.0;
    double mu;
    double lambda;
    double pkt_bits = 500.0 * 8.0;
    Link *l;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        if (sim->graph.link_used[i] && sim->graph.links[i].utilization > best_util) {
            best_util = sim->graph.links[i].utilization;
            best = i;
        }
    }
    if (best < 0) {
        return;
    }
    l = &sim->graph.links[best];
    if (l->qdelay_n > 0) {
        sim->tel.mm1_measured_qdelay = (l->qdelay_sum / (double)l->qdelay_n) * 1000.0;
    }
    mu = (l->bandwidth_kbps * 1000.0) / pkt_bits;
    lambda = 0.0;
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        if (sim->flows[i].used && sim->flows[i].type == FLOW_POISSON) {
            lambda += (sim->flows[i].rate_kbps * 1000.0) /
                      (sim->flows[i].packet_size * 8.0);
        }
    }
    if (lambda > 0.0 && mu > lambda) {
        sim->tel.mm1_theory_qdelay = (lambda / (mu * (mu - lambda))) * 1000.0;
    }
}

void telemetry_print_summary(Simulator *sim, const char *scenario)
{
    double avg, mn, mx;
    double pdr;
    double thr;
    int i;
    char path[256];
    telemetry_mm1(sim);
    telemetry_latency_ms(&sim->tel, &avg, &mn, &mx);
    pdr = telemetry_pdr(&sim->tel);
    thr = telemetry_throughput_kbps(&sim->tel, sim->now > 0 ? sim->now : sim->end_time);
    routing_format_path(&sim->route, path, sizeof(path));
    printf("\n=== %s seed=%u t=%.3fs ===\n", scenario ? scenario : "run", sim->seed,
           sim->now);
    printf("algo=%s  cost_mode=%d  path=%s  path_cost=%.3f\n",
           sim->algo == ROUTING_BELLMAN_FORD ? "Bellman-Ford" : "Dijkstra",
           (int)sim->cost_mode, path, sim->route.cost);
    printf("%-12s %10s %10s %10s %10s %10s %10s\n", "scope", "sent", "recv", "drop",
           "PDR%", "avg_ms", "kbps");
    printf("%-12s %10d %10d %10d %10.2f %10.3f %10.3f\n", "global", sim->tel.sent,
           sim->tel.recv, sim->tel.dropped, pdr, avg, thr);
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        Flow *f = &sim->flows[i];
        double fpdr;
        double favg;
        double fthr;
        double dur;
        if (!f->used) {
            continue;
        }
        fpdr = f->sent ? 100.0 * (double)f->recv / (double)f->sent : 0.0;
        favg = f->lat_n ? (f->lat_sum / (double)f->lat_n) * 1000.0 : 0.0;
        dur = (f->last_recv > f->first_recv) ? (f->last_recv - f->first_recv) : sim->now;
        if (dur < 1e-9) {
            dur = sim->now;
        }
        fthr = (f->bytes_recv * 8.0) / (dur * 1000.0);
        printf("flow%-8d %10d %10d %10d %10.2f %10.3f %10.3f\n", i, f->sent, f->recv,
               f->dropped, fpdr, favg, fthr);
    }
    printf("jitter_ms=%.3f  min_ms=%.3f  max_ms=%.3f  in_flight=%d\n",
           sim->tel.lat_n > 1 ? (sim->tel.jitter_sum / (double)(sim->tel.lat_n - 1)) * 1000.0
                              : 0.0,
           mn, mx, sim->pool.live);
    if (sim->tel.fail_time > 0.0) {
        printf("failure_t=%.3f  reroute_s=%.6f  fail_link=%d\n", sim->tel.fail_time,
               sim->tel.reroute_time, sim->tel.fail_link);
    }
    if (sim->tel.mm1_measured_qdelay >= 0.0) {
        printf("M/M/1 queueing delay: measured=%.4f ms  theory=%.4f ms\n",
               sim->tel.mm1_measured_qdelay, sim->tel.mm1_theory_qdelay);
    }
    printf("Dijkstra vs Bellman-Ford: cost %.3f vs %.3f  settled %d vs iter %d  us %ld vs %ld\n",
           sim->cmp_dij.cost, sim->cmp_bf.cost, sim->cmp_dij.settled, sim->cmp_bf.iterations,
           sim->cmp_dij.runtime_us, sim->cmp_bf.runtime_us);
}
