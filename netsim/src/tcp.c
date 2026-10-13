#include "tcp.h"
#include "sim.h"

void tcp_init_flow(Flow *f)
{
    if (!f) {
        return;
    }
    f->cwnd = 1.0;
    f->ssthresh = 16.0;
    f->in_flight = 0;
    f->snd_una = 0;
    f->snd_next = 0;
    f->rcv_next = 0;
    f->dup_acks = 0;
    f->srtt = 0.05;
    f->rto = 0.2;
    f->timeout_gen += 1;
}

/* Arm a retransmission timeout for the oldest unacked byte. */
static void tcp_arm_timer(Simulator *sim, Flow *f)
{
    f->timeout_gen += 1;
    events_schedule(&sim->sched, sim->now + f->rto, EV_TCP_TIMEOUT, f->id, f->timeout_gen,
                    0, 0.0);
}

void tcp_try_send(Simulator *sim, int flow_id)
{
    Flow *f = &sim->flows[flow_id];
    int window;
    if (!f->used || f->proto != PROTO_TCP) {
        return;
    }
    if (sim->now > f->stop_time + 1e-12) {
        return;
    }
    window = (int)f->cwnd;
    if (window < 1) {
        window = 1;
    }
    while (f->in_flight < window && f->snd_next < 100000) {
        if (sim_emit_packet(sim, flow_id, 0, f->snd_next, f->packet_size) < 0) {
            break;
        }
        f->snd_next += 1;
        f->in_flight += 1;
        f->seq = f->snd_next;
    }
    if (f->in_flight > 0) {
        tcp_arm_timer(sim, f);
    }
}

void tcp_on_ack(Simulator *sim, int flow_id, int ack_seq)
{
    Flow *f;
    int newly;
    if (flow_id < 0 || flow_id >= NETSIM_MAX_FLOWS) {
        return;
    }
    f = &sim->flows[flow_id];
    if (!f->used) {
        return;
    }
    if (ack_seq > f->snd_una) {
        newly = ack_seq - f->snd_una;
        f->snd_una = ack_seq;
        f->in_flight -= newly;
        if (f->in_flight < 0) {
            f->in_flight = 0;
        }
        f->dup_acks = 0;
        f->acked += newly;
        if (f->cwnd < f->ssthresh) {
            f->cwnd += (double)newly;
        } else {
            f->cwnd += (double)newly / f->cwnd;
        }
        f->srtt = 0.875 * f->srtt + 0.125 * 0.05;
        f->rto = f->srtt * 2.0;
        if (f->rto < 0.05) {
            f->rto = 0.05;
        }
        tcp_try_send(sim, flow_id);
        return;
    }
    f->dup_acks += 1;
    if (f->dup_acks == 3) {
        f->ssthresh = f->cwnd / 2.0;
        if (f->ssthresh < 2.0) {
            f->ssthresh = 2.0;
        }
        f->cwnd = f->ssthresh;
        sim_emit_packet(sim, flow_id, 0, f->snd_una, f->packet_size);
        sim_log(sim, "TCP flow %d fast retransmit seq=%d", flow_id, f->snd_una);
    }
}

void tcp_on_timeout(Simulator *sim, int flow_id, int gen)
{
    Flow *f;
    if (flow_id < 0 || flow_id >= NETSIM_MAX_FLOWS) {
        return;
    }
    f = &sim->flows[flow_id];
    if (!f->used || gen != f->timeout_gen) {
        return;
    }
    f->ssthresh = f->cwnd / 2.0;
    if (f->ssthresh < 2.0) {
        f->ssthresh = 2.0;
    }
    f->cwnd = 1.0;
    f->dup_acks = 0;
    f->rto *= 2.0;
    if (f->rto > 2.0) {
        f->rto = 2.0;
    }
    sim_emit_packet(sim, flow_id, 0, f->snd_una, f->packet_size);
    tcp_arm_timer(sim, f);
    sim_log(sim, "TCP flow %d timeout cwnd=1", flow_id);
}
