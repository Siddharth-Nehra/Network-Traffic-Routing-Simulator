#ifndef NETSIM_TCP_H
#define NETSIM_TCP_H

struct Simulator;
struct Flow;
struct Packet;

/* Initialize slow-start state on a TCP flow. */
void tcp_init_flow(struct Flow *f);

/* Try to send packets up to cwnd. */
void tcp_try_send(struct Simulator *sim, int flow_id);

/* Process a cumulative ACK (or dup ACK). */
void tcp_on_ack(struct Simulator *sim, int flow_id, int ack_seq);

/* RTO fired: ssthresh = cwnd/2, cwnd = 1. */
void tcp_on_timeout(struct Simulator *sim, int flow_id, int gen);

#endif /* NETSIM_TCP_H */
