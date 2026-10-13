#include "packet.h"

void packet_pool_init(PacketPool *p)
{
    int i;
    if (!p) {
        return;
    }
    memset(p, 0, sizeof(*p));
    p->free_count = NETSIM_MAX_PACKETS;
    for (i = 0; i < NETSIM_MAX_PACKETS; i++) {
        p->free_ids[i] = NETSIM_MAX_PACKETS - 1 - i;
        p->pkts[i].id = -1;
    }
}

int packet_alloc(PacketPool *p)
{
    int idx;
    Packet *pkt;
    if (!p || p->free_count <= 0) {
        return -1;
    }
    p->free_count -= 1;
    idx = p->free_ids[p->free_count];
    pkt = &p->pkts[idx];
    memset(pkt, 0, sizeof(*pkt));
    p->next_serial += 1;
    pkt->id = p->next_serial;
    pkt->in_use = 1;
    pkt->link_id = -1;
    pkt->next_node = -1;
    pkt->flow_id = -1;
    p->live += 1;
    return idx;
}

void packet_free(PacketPool *p, int idx)
{
    if (!p || idx < 0 || idx >= NETSIM_MAX_PACKETS || !p->pkts[idx].in_use) {
        return;
    }
    p->pkts[idx].in_use = 0;
    p->free_ids[p->free_count] = idx;
    p->free_count += 1;
    p->live -= 1;
}

Packet *packet_at(PacketPool *p, int idx)
{
    if (!p || idx < 0 || idx >= NETSIM_MAX_PACKETS || !p->pkts[idx].in_use) {
        return NULL;
    }
    return &p->pkts[idx];
}

const Packet *packet_at_const(const PacketPool *p, int idx)
{
    if (!p || idx < 0 || idx >= NETSIM_MAX_PACKETS || !p->pkts[idx].in_use) {
        return NULL;
    }
    return &p->pkts[idx];
}
