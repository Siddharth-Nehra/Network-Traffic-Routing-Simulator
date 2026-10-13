#include "sim.h"

#include "io.h"
#include "tcp.h"

#include <stdarg.h>

static void sim_reset_links(Simulator *sim);
static void sim_dispatch(Simulator *sim, const SimEvent *ev);
static int sim_forward(Simulator *sim, int pkt_idx);
static void sim_drop_idx(Simulator *sim, int pkt_idx, const char *why);
static void sim_start_tx(Simulator *sim, int link_id);
static void sim_arm_clock(Simulator *sim);

/* Allocate a zeroed simulator and seed the RNG. */
Simulator *sim_create(uint32_t seed)
{
    Simulator *sim = (Simulator *)calloc(1, sizeof(Simulator));
    if (!sim) {
        return NULL;
    }
    graph_init(&sim->graph);
    packet_pool_init(&sim->pool);
    if (events_init(&sim->sched) != 0) {
        free(sim);
        return NULL;
    }
    telemetry_init(&sim->tel);
    sim->algo = ROUTING_DIJKSTRA;
    sim->cost_mode = COST_STATIC_WEIGHT;
    sim->path_src = -1;
    sim->path_dst = -1;
    sim->selected_node = -1;
    sim->selected_link = -1;
    sim->hover_node = -1;
    sim->hover_link = -1;
    sim->drag_node = -1;
    sim->seed = seed;
    sim->rng_state = seed ? seed : 1u;
    sim->screenshot_n = 1;
    sim->window_w = 1600;
    sim->window_h = 960;
    sim->speed = 1.0;
    sim->end_time = 20.0;
    snprintf(sim->ui.topo_path, sizeof(sim->ui.topo_path), "topologies/session.topo");
    snprintf(sim->ui.in_weight, sizeof(sim->ui.in_weight), "1");
    snprintf(sim->ui.in_bw, sizeof(sim->ui.in_bw), "1000");
    snprintf(sim->ui.in_delay, sizeof(sim->ui.in_delay), "10");
    snprintf(sim->ui.in_loss, sizeof(sim->ui.in_loss), "0");
    snprintf(sim->ui.in_k, sizeof(sim->ui.in_k), "20");
    snprintf(sim->ui.in_fsrc, sizeof(sim->ui.in_fsrc), "0");
    snprintf(sim->ui.in_fdst, sizeof(sim->ui.in_fdst), "2");
    snprintf(sim->ui.in_frate, sizeof(sim->ui.in_frate), "100");
    snprintf(sim->ui.in_fsize, sizeof(sim->ui.in_fsize), "500");
    snprintf(sim->ui.in_fstart, sizeof(sim->ui.in_fstart), "0");
    snprintf(sim->ui.in_fstop, sizeof(sim->ui.in_fstop), "10");
    snprintf(sim->status, sizeof(sim->status),
             "Build a topology, add flows, then press Start.");
    return sim;
}

void sim_destroy(Simulator *sim)
{
    if (!sim) {
        return;
    }
    events_free(&sim->sched);
    graph_free(&sim->graph);
    free(sim);
}

double sim_rand01(Simulator *sim)
{
    uint32_t x;
    if (!sim) {
        return 0.0;
    }
    x = sim->rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    sim->rng_state = x ? x : 1u;
    return (double)sim->rng_state / 4294967296.0;
}

double sim_rand_exp(Simulator *sim, double mean)
{
    double u = sim_rand01(sim);
    if (u < 1e-12) {
        u = 1e-12;
    }
    return -mean * log(u);
}

void sim_log(Simulator *sim, const char *fmt, ...)
{
    char line[128];
    va_list ap;
    int i;
    if (!sim) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    if (sim->ui.log_n < 16) {
        snprintf(sim->ui.log[sim->ui.log_n], sizeof(sim->ui.log[0]), "%s", line);
        sim->ui.log_n += 1;
        return;
    }
    for (i = 0; i < 15; i++) {
        memcpy(sim->ui.log[i], sim->ui.log[i + 1], sizeof(sim->ui.log[0]));
    }
    snprintf(sim->ui.log[15], sizeof(sim->ui.log[0]), "%s", line);
}

static void sim_reset_links(Simulator *sim)
{
    int i;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        Link *l;
        if (!sim->graph.link_used[i]) {
            continue;
        }
        l = &sim->graph.links[i];
        l->queue.head = 0;
        l->queue.tail = 0;
        l->queue.count = 0;
        l->tx_pkt = -1;
        l->busy_until = 0.0;
        l->bytes_sent = 0;
        l->qdelay_sum = 0.0;
        l->qdelay_n = 0;
        l->occupancy_sum = 0.0;
        l->occupancy_n = 0;
        l->utilization = 0.0;
    }
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        if (sim->graph.node_used[i]) {
            sim->graph.nodes[i].packets_forwarded = 0;
            sim->graph.nodes[i].packets_dropped = 0;
        }
    }
}

static void sim_arm_clock(Simulator *sim)
{
    int i;
    events_schedule(&sim->sched, 0.1, EV_SAMPLE, 0, 0, 0, 0.0);
    events_schedule(&sim->sched, sim->end_time, EV_END, 0, 0, 0, 0.0);
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        Flow *f = &sim->flows[i];
        if (f->used) {
            events_schedule(&sim->sched, f->start_time, EV_FLOW_SEND, i, 0, 0, 0.0);
        }
    }
    sim->sample_armed = 1;
}

void sim_reset_runtime(Simulator *sim)
{
    int i;
    if (!sim) {
        return;
    }
    packet_pool_init(&sim->pool);
    events_clear(&sim->sched);
    telemetry_init(&sim->tel);
    sim->now = 0.0;
    sim->playing = 0;
    sim->paused = 1;
    sim_reset_links(sim);
    memset(sim->flashes, 0, sizeof(sim->flashes));
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        Flow *f = &sim->flows[i];
        if (!f->used) {
            continue;
        }
        f->seq = 0;
        f->sent = f->recv = f->dropped = f->acked = 0;
        f->lat_sum = 0.0;
        f->lat_min = 1e9;
        f->lat_max = 0.0;
        f->lat_n = 0;
        f->bytes_recv = f->bytes_sent = 0.0;
        f->first_recv = f->last_recv = 0.0;
        tcp_init_flow(f);
    }
    sim_arm_clock(sim);
    sim_recompute_route(sim);
}

void sim_flash_drop(Simulator *sim, float x, float y)
{
    int i;
    for (i = 0; i < NETSIM_MAX_FLASH; i++) {
        if (!sim->flashes[i].used || sim->flashes[i].until < sim->now) {
            sim->flashes[i].used = 1;
            sim->flashes[i].x = x;
            sim->flashes[i].y = y;
            sim->flashes[i].until = sim->now + 0.35;
            return;
        }
    }
}

static int sim_fill_tables(Simulator *sim)
{
    int u;
    int v;
    for (u = 0; u < NETSIM_MAX_NODES; u++) {
        for (v = 0; v < NETSIM_MAX_NODES; v++) {
            sim->next_hop[u][v] = -1;
            sim->rt_cost[u][v] = NETSIM_INF;
        }
    }
    for (u = 0; u < NETSIM_MAX_NODES; u++) {
        RoutingResult tmp;
        int rc;
        if (!sim->graph.node_used[u]) {
            continue;
        }
        if (sim->algo == ROUTING_BELLMAN_FORD) {
            rc = routing_bellman_ford(&sim->graph, u, sim->cost_mode, &tmp);
        } else {
            rc = routing_dijkstra(&sim->graph, u, sim->cost_mode, &tmp);
        }
        if (rc != 0) {
            return rc;
        }
        for (v = 0; v < NETSIM_MAX_NODES; v++) {
            sim->next_hop[u][v] = tmp.next_hop[v];
            sim->rt_cost[u][v] = tmp.dist[v];
        }
    }
    return 0;
}

int sim_recompute_route(Simulator *sim)
{
    char pathbuf[256];
    const char *aname;
    if (!sim) {
        return -1;
    }
    if (sim_fill_tables(sim) != 0) {
        return -1;
    }
    if (!graph_node_exists(&sim->graph, sim->path_src) ||
        !graph_node_exists(&sim->graph, sim->path_dst)) {
        memset(&sim->route, 0, sizeof(sim->route));
        sim->route.cost = NETSIM_INF;
        return 0;
    }
    routing_shortest_path(&sim->graph, sim->path_src, sim->path_dst, sim->cost_mode,
                          ROUTING_DIJKSTRA, &sim->cmp_dij);
    routing_shortest_path(&sim->graph, sim->path_src, sim->path_dst, sim->cost_mode,
                          ROUTING_BELLMAN_FORD, &sim->cmp_bf);
    routing_shortest_path(&sim->graph, sim->path_src, sim->path_dst, sim->cost_mode,
                          sim->algo, &sim->route);
    routing_format_path(&sim->route, pathbuf, sizeof(pathbuf));
    aname = (sim->algo == ROUTING_BELLMAN_FORD) ? "Bellman-Ford" : "Dijkstra";
    snprintf(sim->status, sizeof(sim->status),
             "%s  src=%d dst=%d  cost=%.3f  path %s", aname, sim->path_src,
             sim->path_dst, netsim_finite(sim->route.cost) ? sim->route.cost : -1.0,
             pathbuf);
    return 0;
}

int sim_load_demo4(Simulator *sim)
{
    int n0, n1, n2, n3;
    if (!sim) {
        return -1;
    }
    graph_free(&sim->graph);
    graph_init(&sim->graph);
    n0 = graph_add_node(&sim->graph, 260.0f, 180.0f);
    n1 = graph_add_node(&sim->graph, 700.0f, 180.0f);
    n2 = graph_add_node(&sim->graph, 700.0f, 480.0f);
    n3 = graph_add_node(&sim->graph, 260.0f, 480.0f);
    if (n0 < 0 || n1 < 0 || n2 < 0 || n3 < 0) {
        return -1;
    }
    if (graph_add_link(&sim->graph, n0, n1, 1.0, 1000.0, 10.0, 0.0, 20) < 0 ||
        graph_add_link(&sim->graph, n1, n2, 1.0, 1000.0, 10.0, 0.0, 20) < 0 ||
        graph_add_link(&sim->graph, n2, n3, 4.0, 1000.0, 10.0, 0.0, 20) < 0 ||
        graph_add_link(&sim->graph, n3, n0, 1.0, 1000.0, 10.0, 0.0, 20) < 0 ||
        graph_add_link(&sim->graph, n0, n2, 10.0, 1000.0, 10.0, 0.0, 20) < 0) {
        return -1;
    }
    sim->path_src = n0;
    sim->path_dst = n2;
    return sim_recompute_route(sim);
}

/* Drop a live packet and update counters. */
static void sim_drop_idx(Simulator *sim, int pkt_idx, const char *why)
{
    Packet *p = packet_at(&sim->pool, pkt_idx);
    Node *n;
    (void)why;
    if (!p) {
        return;
    }
    n = graph_node(&sim->graph, p->current_node);
    if (n) {
        n->packets_dropped += 1;
        sim_flash_drop(sim, n->x, n->y);
    }
    telemetry_on_drop(sim, p->flow_id);
    packet_free(&sim->pool, pkt_idx);
}

/* Begin serializing the head-of-line packet on a link. */
static void sim_start_tx(Simulator *sim, int link_id)
{
    Link *l = graph_link(&sim->graph, link_id);
    Packet *p;
    int idx;
    double service;
    if (!l || l->tx_pkt >= 0 || l->state != LINK_UP) {
        return;
    }
    if (queue_pop(&l->queue, &idx) != 0) {
        return;
    }
    p = packet_at(&sim->pool, idx);
    if (!p) {
        return;
    }
    service = (p->size_bytes * 8.0) / (l->bandwidth_kbps * 1000.0);
    if (service < 1e-9) {
        service = 1e-9;
    }
    p->tx_start = sim->now;
    l->qdelay_sum += (sim->now - p->enqueue_time);
    l->qdelay_n += 1;
    l->bytes_sent += (uint64_t)p->size_bytes;
    l->tx_pkt = idx;
    l->busy_until = sim->now + service;
    events_schedule(&sim->sched, l->busy_until, EV_TX_DONE, link_id, idx, 0, 0.0);
}

/* Queue a packet on a directed link or tail-drop it. */
static int sim_enqueue(Simulator *sim, int pkt_idx, int link_id)
{
    Link *l = graph_link(&sim->graph, link_id);
    Packet *p = packet_at(&sim->pool, pkt_idx);
    if (!l || !p || l->state != LINK_UP) {
        sim_drop_idx(sim, pkt_idx, "link-down");
        return -1;
    }
    p->link_id = link_id;
    p->current_node = l->src;
    p->next_node = l->dst;
    p->enqueue_time = sim->now;
    l->occupancy_sum += (double)queue_len(&l->queue);
    l->occupancy_n += 1;
    if (queue_push(&l->queue, pkt_idx) != 0) {
        sim_drop_idx(sim, pkt_idx, "tail-drop");
        return -1;
    }
    if (l->tx_pkt < 0) {
        sim_start_tx(sim, link_id);
    }
    return 0;
}

static int sim_forward(Simulator *sim, int pkt_idx)
{
    Packet *p = packet_at(&sim->pool, pkt_idx);
    int nh;
    int link;
    Node *n;
    if (!p) {
        return -1;
    }
    n = graph_node(&sim->graph, p->current_node);
    if (!n || n->state != NODE_UP) {
        sim_drop_idx(sim, pkt_idx, "node-down");
        return -1;
    }
    nh = sim->next_hop[p->current_node][p->dst];
    if (nh < 0 || nh == p->current_node) {
        sim_drop_idx(sim, pkt_idx, "no-route");
        return -1;
    }
    link = graph_find_link(&sim->graph, p->current_node, nh);
    if (link < 0) {
        sim_drop_idx(sim, pkt_idx, "no-link");
        return -1;
    }
    n->packets_forwarded += 1;
    return sim_enqueue(sim, pkt_idx, link);
}

int sim_emit_packet(Simulator *sim, int flow_id, int is_ack, int seq, int size)
{
    Flow *f;
    Packet *p;
    int idx;
    if (flow_id < 0 || flow_id >= NETSIM_MAX_FLOWS || !sim->flows[flow_id].used) {
        return -1;
    }
    f = &sim->flows[flow_id];
    idx = packet_alloc(&sim->pool);
    if (idx < 0) {
        telemetry_on_drop(sim, flow_id);
        return -1;
    }
    p = packet_at(&sim->pool, idx);
    p->flow_id = flow_id;
    p->proto = f->proto;
    p->size_bytes = size > 0 ? size : f->packet_size;
    p->created_at = sim->now;
    p->seq = seq;
    p->is_ack = is_ack;
    p->hop_count = 0;
    if (is_ack) {
        p->src = f->dst;
        p->dst = f->src;
        p->current_node = f->dst;
    } else {
        p->src = f->src;
        p->dst = f->dst;
        p->current_node = f->src;
        f->sent += 1;
        f->bytes_sent += (double)p->size_bytes;
        sim->tel.sent += 1;
    }
    return sim_forward(sim, idx);
}

static void sim_maybe_reroute_mark(Simulator *sim, const Packet *p)
{
    if (!sim->tel.waiting_reroute || p->is_ack) {
        return;
    }
    sim->tel.reroute_time = sim->now - sim->tel.fail_time;
    sim->tel.waiting_reroute = 0;
    sim_log(sim, "reroute deliver t=%.3fs dt=%.4fs", sim->now, sim->tel.reroute_time);
}

static void sim_deliver(Simulator *sim, int pkt_idx)
{
    Packet *p = packet_at(&sim->pool, pkt_idx);
    Flow *f;
    double lat;
    if (!p) {
        return;
    }
    f = (p->flow_id >= 0) ? &sim->flows[p->flow_id] : NULL;
    lat = sim->now - p->created_at;
    if (p->is_ack && f) {
        tcp_on_ack(sim, p->flow_id, p->seq);
        packet_free(&sim->pool, pkt_idx);
        return;
    }
    if (f) {
        telemetry_on_deliver(sim, p->flow_id, lat);
        if (f->last_path_len <= 0) {
            routing_build_path(&sim->route, f->dst);
        }
        sim_maybe_reroute_mark(sim, p);
        if (f->proto == PROTO_TCP) {
            if (p->seq == f->rcv_next) {
                f->rcv_next += 1;
            }
            sim_emit_packet(sim, p->flow_id, 1, f->rcv_next, 40);
        }
    }
    packet_free(&sim->pool, pkt_idx);
}

static void sim_on_tx_done(Simulator *sim, int link_id, int pkt_idx)
{
    Link *l = graph_link(&sim->graph, link_id);
    Packet *p = packet_at(&sim->pool, pkt_idx);
    if (l && l->tx_pkt == pkt_idx) {
        l->tx_pkt = -1;
    }
    if (!p) {
        if (l) {
            sim_start_tx(sim, link_id);
        }
        return;
    }
    if (!l || l->state != LINK_UP || sim_rand01(sim) < l->loss_rate) {
        sim_drop_idx(sim, pkt_idx, "loss");
        if (l) {
            sim_start_tx(sim, link_id);
        }
        return;
    }
    p->arrive_at = sim->now + l->prop_delay_ms / 1000.0;
    events_schedule(&sim->sched, p->arrive_at, EV_PKT_ARRIVE, pkt_idx, link_id, 0, 0.0);
    sim_start_tx(sim, link_id);
}

static void sim_on_arrive(Simulator *sim, int pkt_idx, int link_id)
{
    Packet *p = packet_at(&sim->pool, pkt_idx);
    Link *l = graph_link(&sim->graph, link_id);
    if (!p) {
        return;
    }
    if (!l || l->state != LINK_UP) {
        sim_drop_idx(sim, pkt_idx, "dead-link");
        return;
    }
    p->current_node = l->dst;
    p->hop_count += 1;
    p->link_id = -1;
    if (p->current_node == p->dst) {
        sim_deliver(sim, pkt_idx);
        return;
    }
    sim_forward(sim, pkt_idx);
}

static void sim_purge_link_packets(Simulator *sim, int link_id)
{
    Link *l = graph_link(&sim->graph, link_id);
    int idx;
    int i;
    if (!l) {
        return;
    }
    if (l->tx_pkt >= 0) {
        sim_drop_idx(sim, l->tx_pkt, "fail-tx");
        l->tx_pkt = -1;
    }
    while (queue_pop(&l->queue, &idx) == 0) {
        sim_drop_idx(sim, idx, "fail-q");
    }
    for (i = 0; i < NETSIM_MAX_PACKETS; i++) {
        Packet *p = packet_at(&sim->pool, i);
        if (p && p->link_id == link_id) {
            sim_drop_idx(sim, i, "fail-flight");
        }
    }
}

void sim_on_topology_failure(Simulator *sim, int link_id)
{
    Link *l = graph_link(&sim->graph, link_id);
    int rev;
    sim->tel.fail_time = sim->now;
    sim->tel.fail_link = link_id;
    sim->tel.waiting_reroute = 1;
    sim->tel.reroute_time = -1.0;
    memcpy(sim->tel.old_path, sim->route.path, sizeof(sim->tel.old_path));
    sim->tel.old_path_len = sim->route.path_len;
    if (l) {
        sim_purge_link_packets(sim, link_id);
        rev = l->reverse_id;
        if (rev >= 0) {
            sim_purge_link_packets(sim, rev);
        }
    }
    sim_recompute_route(sim);
    memcpy(sim->tel.new_path, sim->route.path, sizeof(sim->tel.new_path));
    sim->tel.new_path_len = sim->route.path_len;
    sim_log(sim, "link %d failed at t=%.3f", link_id, sim->now);
}

void sim_fail_selected(Simulator *sim)
{
    if (sim->selected_link >= 0 && graph_link_exists(&sim->graph, sim->selected_link)) {
        Link *l = graph_link(&sim->graph, sim->selected_link);
        LinkState next = (l->state == LINK_UP) ? LINK_DOWN : LINK_UP;
        graph_set_link_state(&sim->graph, sim->selected_link, next);
        if (next == LINK_DOWN) {
            sim_on_topology_failure(sim, sim->selected_link);
        } else {
            sim_recompute_route(sim);
            sim_log(sim, "link %d restored", sim->selected_link);
        }
        return;
    }
    if (sim->selected_node >= 0 && graph_node_exists(&sim->graph, sim->selected_node)) {
        Node *n = graph_node(&sim->graph, sim->selected_node);
        NodeState next = (n->state == NODE_UP) ? NODE_DOWN : NODE_UP;
        graph_set_node_state(&sim->graph, sim->selected_node, next);
        sim_recompute_route(sim);
        sim_log(sim, "node %d %s", sim->selected_node, next == NODE_DOWN ? "DOWN" : "UP");
    }
}

static void sim_update_util(Simulator *sim)
{
    int i;
    double t = sim->now > 0.0 ? sim->now : 1e-9;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        Link *l;
        double bits;
        if (!sim->graph.link_used[i]) {
            continue;
        }
        l = &sim->graph.links[i];
        bits = (double)l->bytes_sent * 8.0;
        l->utilization = bits / (l->bandwidth_kbps * 1000.0 * t);
        if (l->utilization > 1.0) {
            l->utilization = 1.0;
        }
    }
}

static void sim_dispatch(Simulator *sim, const SimEvent *ev)
{
    sim->now = ev->time;
    switch (ev->type) {
    case EV_FLOW_SEND:
        traffic_on_send(sim, ev->a);
        break;
    case EV_TX_DONE:
        sim_on_tx_done(sim, ev->a, ev->b);
        break;
    case EV_PKT_ARRIVE:
        sim_on_arrive(sim, ev->a, ev->b);
        break;
    case EV_LINK_FAIL:
        graph_set_link_state(&sim->graph, ev->a, LINK_DOWN);
        sim_on_topology_failure(sim, ev->a);
        break;
    case EV_LINK_RESTORE:
        graph_set_link_state(&sim->graph, ev->a, LINK_UP);
        sim_recompute_route(sim);
        break;
    case EV_NODE_FAIL:
        graph_set_node_state(&sim->graph, ev->a, NODE_DOWN);
        sim_recompute_route(sim);
        break;
    case EV_NODE_RESTORE:
        graph_set_node_state(&sim->graph, ev->a, NODE_UP);
        sim_recompute_route(sim);
        break;
    case EV_WEIGHT_CHANGE:
        graph_set_weight(&sim->graph, ev->a, ev->value);
        sim_recompute_route(sim);
        break;
    case EV_TCP_TIMEOUT:
        tcp_on_timeout(sim, ev->a, ev->b);
        break;
    case EV_SAMPLE:
        telemetry_sample(sim);
        sim_update_util(sim);
        if (sim->now < sim->end_time - 1e-9) {
            events_schedule(&sim->sched, sim->now + 0.1, EV_SAMPLE, 0, 0, 0, 0.0);
        }
        break;
    case EV_END:
        sim->playing = 0;
        sim->paused = 1;
        telemetry_sample(sim);
        sim_update_util(sim);
        break;
    default:
        break;
    }
}

int sim_step(Simulator *sim)
{
    SimEvent ev;
    if (!sim || events_pop(&sim->sched, &ev) != 0) {
        return 0;
    }
    if (ev.time > sim->end_time + 1e-9) {
        sim->playing = 0;
        return 0;
    }
    sim_dispatch(sim, &ev);
    return 1;
}

void sim_advance(Simulator *sim, double dt)
{
    double target;
    if (!sim || dt <= 0.0) {
        return;
    }
    target = sim->now + dt;
    if (target > sim->end_time) {
        target = sim->end_time;
    }
    for (;;) {
        double next_t;
        if (!events_peek_time(&sim->sched, &next_t)) {
            sim->now = target;
            break;
        }
        if (next_t > target + 1e-12) {
            sim->now = target;
            break;
        }
        if (!sim_step(sim)) {
            break;
        }
    }
}

void sim_play(Simulator *sim)
{
    if (!sim) {
        return;
    }
    if (!sim->sample_armed) {
        sim_arm_clock(sim);
    }
    sim->playing = 1;
    sim->paused = 0;
}

void sim_pause(Simulator *sim)
{
    if (sim) {
        sim->playing = 0;
        sim->paused = 1;
    }
}

int sim_apply_link_fields(Simulator *sim)
{
    double w, bw, dly, loss;
    int k;
    if (!sim || sim->selected_link < 0) {
        return -1;
    }
    w = strtod(sim->ui.in_weight, NULL);
    bw = strtod(sim->ui.in_bw, NULL);
    dly = strtod(sim->ui.in_delay, NULL);
    loss = strtod(sim->ui.in_loss, NULL);
    k = (int)strtol(sim->ui.in_k, NULL, 10);
    if (graph_set_link_params(&sim->graph, sim->selected_link, w, bw, dly, loss, k) !=
        0) {
        return -1;
    }
    return sim_recompute_route(sim);
}

int sim_apply_flow_form(Simulator *sim)
{
    int src = (int)strtol(sim->ui.in_fsrc, NULL, 10);
    int dst = (int)strtol(sim->ui.in_fdst, NULL, 10);
    double rate = strtod(sim->ui.in_frate, NULL);
    int size = (int)strtol(sim->ui.in_fsize, NULL, 10);
    double st = strtod(sim->ui.in_fstart, NULL);
    double sp = strtod(sim->ui.in_fstop, NULL);
    FlowType type = (FlowType)sim->ui.flow_type;
    int id = traffic_add_flow(sim, src, dst, type, rate, size, st, sp);
    if (id < 0) {
        sim_log(sim, "could not add flow");
        return -1;
    }
    sim_log(sim, "flow %d %d->%d added", id, src, dst);
    return 0;
}

