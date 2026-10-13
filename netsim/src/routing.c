#include "routing.h"
#include "heap.h"

double routing_edge_cost(const Graph *g, int link_id, CostMode mode)
{
    const Link *l = graph_link_const(g, link_id);
    if (!l || l->state != LINK_UP) {
        return NETSIM_INF;
    }
    if (mode == COST_HOP_COUNT) {
        return 1.0;
    }
    if (mode == COST_DYNAMIC) {
        return l->prop_delay_ms + (double)queue_len(&l->queue);
    }
    return l->weight;
}

/* True if both endpoints and the link are UP. */
static int routing_edge_usable(const Graph *g, int link_id)
{
    const Link *l = graph_link_const(g, link_id);
    const Node *u;
    const Node *v;
    if (!l || l->state != LINK_UP) {
        return 0;
    }
    u = graph_node_const(g, l->src);
    v = graph_node_const(g, l->dst);
    return u && v && u->state == NODE_UP && v->state == NODE_UP;
}

/* Clear a result struct and set distances to infinity. */
static void routing_result_reset(RoutingResult *out, int src, RoutingAlgo algo)
{
    int i;
    memset(out, 0, sizeof(*out));
    out->src = src;
    out->dst = -1;
    out->cost = NETSIM_INF;
    out->path_len = 0;
    out->algo = algo;
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        out->next_hop[i] = -1;
        out->dist[i] = NETSIM_INF;
        out->prev[i] = -1;
    }
    if (src >= 0 && src < NETSIM_MAX_NODES) {
        out->dist[src] = 0.0;
        out->next_hop[src] = src;
    }
}

/* Derive the first hop toward every destination from prev[]. */
static void routing_fill_next_hops(RoutingResult *out, int src)
{
    int v;
    for (v = 0; v < NETSIM_MAX_NODES; v++) {
        int cur;
        if (v == src) {
            out->next_hop[v] = src;
            continue;
        }
        if (out->prev[v] < 0 || !netsim_finite(out->dist[v])) {
            out->next_hop[v] = -1;
            continue;
        }
        cur = v;
        while (out->prev[cur] >= 0 && out->prev[cur] != src) {
            cur = out->prev[cur];
        }
        out->next_hop[v] = (out->prev[cur] == src) ? cur : -1;
    }
}

/* Relax outgoing edges of a settled node. Returns -1 if the heap grows fail. */
static int dijkstra_relax(const Graph *g, CostMode mode, RoutingResult *out, Heap *heap,
                          int u)
{
    const AdjListNode *edge;
    for (edge = g->adj[u]; edge; edge = edge->next) {
        int v = edge->neighbor;
        double w;
        double cand;
        if (!routing_edge_usable(g, edge->link_id)) {
            continue;
        }
        w = routing_edge_cost(g, edge->link_id, mode);
        if (!netsim_finite(w)) {
            continue;
        }
        cand = out->dist[u] + w;
        if (cand + NETSIM_EPS < out->dist[v]) {
            out->dist[v] = cand;
            out->prev[v] = u;
            if (heap_push(heap, cand, v, NULL) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

int routing_dijkstra(const Graph *g, int src, CostMode mode, RoutingResult *out)
{
    Heap heap;
    HeapItem item;
    long t0;
    int seen[NETSIM_MAX_NODES];
    if (!g || !out || !graph_node_exists(g, src)) {
        return -1;
    }
    routing_result_reset(out, src, ROUTING_DIJKSTRA);
    memset(seen, 0, sizeof(seen));
    if (g->nodes[src].state != NODE_UP) {
        return 0;
    }
    if (heap_init(&heap) != 0) {
        return -1;
    }
    t0 = netsim_now_us();
    if (heap_push(&heap, 0.0, src, NULL) != 0) {
        heap_free(&heap);
        return -1;
    }
    while (!heap_empty(&heap)) {
        int u;
        if (heap_pop(&heap, &item) != 0) {
            break;
        }
        u = item.value;
        if (seen[u]) {
            continue;
        }
        seen[u] = 1;
        out->settled += 1;
        if (dijkstra_relax(g, mode, out, &heap, u) != 0) {
            heap_free(&heap);
            return -1;
        }
    }
    out->runtime_us = netsim_now_us() - t0;
    if (out->runtime_us < 0) {
        out->runtime_us = 0;
    }
    routing_fill_next_hops(out, src);
    heap_free(&heap);
    return 0;
}

int routing_build_path(RoutingResult *out, int dst)
{
    int stack[NETSIM_MAX_PATH];
    int n = 0;
    int cur;
    int i;
    if (!out || dst < 0 || dst >= NETSIM_MAX_NODES) {
        return -1;
    }
    out->dst = dst;
    out->path_len = 0;
    out->cost = out->dist[dst];
    if (!netsim_finite(out->dist[dst])) {
        return 0;
    }
    cur = dst;
    while (cur >= 0 && n < NETSIM_MAX_PATH) {
        stack[n++] = cur;
        if (cur == out->src) {
            break;
        }
        cur = out->prev[cur];
    }
    if (n == 0 || stack[n - 1] != out->src) {
        out->cost = NETSIM_INF;
        return 0;
    }
    for (i = 0; i < n; i++) {
        out->path[i] = stack[n - 1 - i];
    }
    out->path_len = n;
    return 0;
}

int routing_bellman_ford(const Graph *g, int src, CostMode mode, RoutingResult *out)
{
    int vcount = 0;
    int i;
    int u;
    int pass;
    long t0;
    if (!g || !out || !graph_node_exists(g, src)) {
        return -1;
    }
    routing_result_reset(out, src, ROUTING_BELLMAN_FORD);
    if (g->nodes[src].state != NODE_UP) {
        return 0;
    }
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        if (g->node_used[i]) {
            vcount += 1;
        }
    }
    t0 = netsim_now_us();
    for (pass = 0; pass < vcount - 1; pass++) {
        int changed = 0;
        for (u = 0; u < NETSIM_MAX_NODES; u++) {
            const AdjListNode *edge;
            if (!g->node_used[u] || !netsim_finite(out->dist[u])) {
                continue;
            }
            for (edge = g->adj[u]; edge; edge = edge->next) {
                double w;
                double cand;
                int v = edge->neighbor;
                if (!routing_edge_usable(g, edge->link_id)) {
                    continue;
                }
                w = routing_edge_cost(g, edge->link_id, mode);
                if (!netsim_finite(w)) {
                    continue;
                }
                cand = out->dist[u] + w;
                if (cand + NETSIM_EPS < out->dist[v]) {
                    out->dist[v] = cand;
                    out->prev[v] = u;
                    changed = 1;
                }
            }
        }
        out->iterations += 1;
        if (!changed) {
            break;
        }
    }
    for (u = 0; u < NETSIM_MAX_NODES && !out->negative_cycle; u++) {
        const AdjListNode *edge;
        if (!g->node_used[u] || !netsim_finite(out->dist[u])) {
            continue;
        }
        for (edge = g->adj[u]; edge; edge = edge->next) {
            double w;
            if (!routing_edge_usable(g, edge->link_id)) {
                continue;
            }
            w = routing_edge_cost(g, edge->link_id, mode);
            if (netsim_finite(w) && out->dist[u] + w + NETSIM_EPS < out->dist[edge->neighbor]) {
                out->negative_cycle = 1;
                break;
            }
        }
    }
    out->settled = vcount;
    out->runtime_us = netsim_now_us() - t0;
    if (out->runtime_us < 0) {
        out->runtime_us = 0;
    }
    routing_fill_next_hops(out, src);
    return 0;
}

int routing_shortest_path(const Graph *g, int src, int dst, CostMode mode,
                          RoutingAlgo algo, RoutingResult *out)
{
    int rc;
    if (algo == ROUTING_BELLMAN_FORD) {
        rc = routing_bellman_ford(g, src, mode, out);
    } else {
        rc = routing_dijkstra(g, src, mode, out);
    }
    if (rc != 0) {
        return rc;
    }
    return routing_build_path(out, dst);
}

void routing_format_path(const RoutingResult *r, char *buf, size_t buflen)
{
    int i;
    size_t used = 0;
    if (!r || !buf || buflen == 0) {
        return;
    }
    buf[0] = '\0';
    if (r->path_len <= 0) {
        snprintf(buf, buflen, "(unreachable)");
        return;
    }
    for (i = 0; i < r->path_len; i++) {
        int n;
        if (i == 0) {
            n = snprintf(buf + used, buflen - used, "%d", r->path[i]);
        } else {
            n = snprintf(buf + used, buflen - used, " -> %d", r->path[i]);
        }
        if (n < 0) {
            break;
        }
        used += (size_t)n;
        if (used >= buflen) {
            buf[buflen - 1] = '\0';
            break;
        }
    }
}
