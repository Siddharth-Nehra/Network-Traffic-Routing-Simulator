#ifndef NETSIM_PACKET_H
#define NETSIM_PACKET_H

#include "common.h"

typedef struct Packet {
    int id;
    int flow_id;
    int src;
    int dst;
    ProtocolKind proto;
    int size_bytes;
    double created_at;
    int seq;
    int current_node;
    int next_node;
    int hop_count;
    int is_ack;
    int in_use;
    int link_id;
    double enqueue_time;
    double tx_start;
    double arrive_at;
    int dropped;
} Packet;

typedef struct PacketPool {
    Packet pkts[NETSIM_MAX_PACKETS];
    int free_ids[NETSIM_MAX_PACKETS];
    int free_count;
    int live;
    int next_serial;
} PacketPool;

/* Set up the free list so every slot is available. */
void packet_pool_init(PacketPool *p);

/* Allocate a packet. Returns index or -1. */
int packet_alloc(PacketPool *p);

/* Return a packet to the pool. */
void packet_free(PacketPool *p, int idx);

/* Pointer to a live packet, or NULL. */
Packet *packet_at(PacketPool *p, int idx);
const Packet *packet_at_const(const PacketPool *p, int idx);

#endif /* NETSIM_PACKET_H */
