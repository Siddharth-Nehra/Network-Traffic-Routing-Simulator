#include "traffic.h"
#include "sim.h"
#include "tcp.h"

double traffic_interval(const Flow *f)
{
    if (!f || f->rate_kbps <= 0.0 || f->packet_size <= 0) {
        return 0.1;
    }
    return (f->packet_size * 8.0) / (f->rate_kbps * 1000.0);
}

int traffic_add_flow(Simulator *sim, int src, int dst, FlowType type, double rate_kbps,
                     int packet_size, double start_t, double stop_t)
{
    int i;
    Flow *f;
    if (!sim || !graph_node_exists(&sim->graph, src) ||
        !graph_node_exists(&sim->graph, dst) || src == dst) {
        return -1;
    }
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        if (!sim->flows[i].used) {
            break;
        }
    }
    if (i >= NETSIM_MAX_FLOWS) {
        return -1;
    }
    f = &sim->flows[i];
    memset(f, 0, sizeof(*f));
    f->id = i;
    f->used = 1;
    f->src = src;
    f->dst = dst;
    f->type = type;
    f->proto = (type == FLOW_TCP) ? PROTO_TCP : PROTO_UDP;
    f->rate_kbps = rate_kbps;
    f->packet_size = packet_size > 0 ? packet_size : NETSIM_DEFAULT_PKT;
    f->start_time = start_t;
    f->stop_time = stop_t > start_t ? stop_t : start_t + 10.0;
    f->on_ms = 200.0;
    f->off_ms = 200.0;
    f->on_state = 1;
    f->state_until = start_t + f->on_ms / 1000.0;
    f->lat_min = 1e9;
    tcp_init_flow(f);
    sim->flow_count += 1;
    events_schedule(&sim->sched, start_t, EV_FLOW_SEND, i, 0, 0, 0.0);
    if (!sim->sample_armed) {
        events_schedule(&sim->sched, 0.1, EV_SAMPLE, 0, 0, 0, 0.0);
        events_schedule(&sim->sched, sim->end_time, EV_END, 0, 0, 0, 0.0);
        sim->sample_armed = 1;
    }
    return i;
}

/* Flip ON/OFF for a bursty source. */
static void traffic_update_burst(Simulator *sim, Flow *f)
{
    while (f->type == FLOW_BURSTY && sim->now >= f->state_until - 1e-12) {
        f->on_state = !f->on_state;
        if (f->on_state) {
            f->state_until += f->on_ms / 1000.0;
        } else {
            f->state_until += f->off_ms / 1000.0;
        }
    }
}

void traffic_on_send(Simulator *sim, int flow_id)
{
    Flow *f;
    double gap;
    if (flow_id < 0 || flow_id >= NETSIM_MAX_FLOWS || !sim->flows[flow_id].used) {
        return;
    }
    f = &sim->flows[flow_id];
    if (sim->now > f->stop_time + 1e-12) {
        f->active = 0;
        return;
    }
    f->active = 1;
    if (f->type == FLOW_TCP) {
        tcp_try_send(sim, flow_id);
        return;
    }
    traffic_update_burst(sim, f);
    if (f->type != FLOW_BURSTY || f->on_state) {
        sim_emit_packet(sim, flow_id, 0, f->seq, f->packet_size);
        f->seq += 1;
    }
    gap = traffic_interval(f);
    if (f->type == FLOW_POISSON) {
        gap = sim_rand_exp(sim, gap);
    }
    if (f->type == FLOW_BURSTY && !f->on_state) {
        gap = f->state_until - sim->now;
        if (gap < 1e-6) {
            gap = 1e-6;
        }
    }
    if (sim->now + gap <= f->stop_time + 1e-9) {
        events_schedule(&sim->sched, sim->now + gap, EV_FLOW_SEND, flow_id, 0, 0, 0.0);
    }
}
