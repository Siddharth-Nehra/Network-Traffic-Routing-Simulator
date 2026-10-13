#include "graph.h"

/* Set one adjacency-matrix row to diagonal 0 / off-diagonal infinity. */
static void graph_reset_matrix_row(Graph *g, int i)
{
    int j;
    for (j = 0; j < NETSIM_MAX_NODES; j++) {
        g->matrix[i][j] = (i == j) ? 0.0 : NETSIM_INF;
        g->matrix_link[i][j] = -1;
    }
}

void graph_init(Graph *g)
{
    int i;
    if (!g) {
        return;
    }
    memset(g, 0, sizeof(*g));
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        g->adj[i] = NULL;
        graph_reset_matrix_row(g, i);
    }
}

void graph_free(Graph *g)
{
    int i;
    if (!g) {
        return;
    }
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        queue_free(&g->links[i].queue);
    }
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        AdjListNode *n = g->adj[i];
        while (n) {
            AdjListNode *next = n->next;
            free(n);
            n = next;
        }
        g->adj[i] = NULL;
    }
}

int graph_node_exists(const Graph *g, int node_id)
{
    return g && node_id >= 0 && node_id < NETSIM_MAX_NODES && g->node_used[node_id];
}

int graph_link_exists(const Graph *g, int link_id)
{
    return g && link_id >= 0 && link_id < NETSIM_MAX_LINKS && g->link_used[link_id];
}

Node *graph_node(Graph *g, int node_id)
{
    return graph_node_exists(g, node_id) ? &g->nodes[node_id] : NULL;
}

const Node *graph_node_const(const Graph *g, int node_id)
{
    return graph_node_exists(g, node_id) ? &g->nodes[node_id] : NULL;
}

Link *graph_link(Graph *g, int link_id)
{
    return graph_link_exists(g, link_id) ? &g->links[link_id] : NULL;
}

const Link *graph_link_const(const Graph *g, int link_id)
{
    return graph_link_exists(g, link_id) ? &g->links[link_id] : NULL;
}

int graph_find_link(const Graph *g, int src, int dst)
{
    if (!graph_node_exists(g, src) || !graph_node_exists(g, dst)) {
        return -1;
    }
    return g->matrix_link[src][dst];
}

/* Find a free node slot, or -1 if the graph is full. */
static int graph_alloc_node_slot(Graph *g)
{
    int i;
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        if (!g->node_used[i]) {
            return i;
        }
    }
    return -1;
}

/* Find a free link slot, or -1 if the graph is full. */
static int graph_alloc_link_slot(Graph *g)
{
    int i;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        if (!g->link_used[i]) {
            return i;
        }
    }
    return -1;
}

int graph_add_node(Graph *g, float x, float y)
{
    int id;
    Node *n;
    if (!g) {
        return -1;
    }
    id = graph_alloc_node_slot(g);
    if (id < 0) {
        return -1;
    }
    n = &g->nodes[id];
    memset(n, 0, sizeof(*n));
    n->id = id;
    snprintf(n->name, sizeof(n->name), "N%d", id);
    n->x = x;
    n->y = y;
    n->state = NODE_UP;
    g->node_used[id] = 1;
    g->node_count += 1;
    graph_reset_matrix_row(g, id);
    g->matrix[id][id] = 0.0;
    return id;
}

/* Prepend a directed adjacency-list edge. */
static int graph_adj_insert(Graph *g, int src, int dst, int link_id)
{
    AdjListNode *node = (AdjListNode *)malloc(sizeof(AdjListNode));
    if (!node) {
        return -1;
    }
    node->neighbor = dst;
    node->link_id = link_id;
    node->next = g->adj[src];
    g->adj[src] = node;
    return 0;
}

/* Remove one directed adjacency-list edge and free it. */
static void graph_adj_remove(Graph *g, int src, int dst, int link_id)
{
    AdjListNode **pp = &g->adj[src];
    while (*pp) {
        if ((*pp)->neighbor == dst && (*pp)->link_id == link_id) {
            AdjListNode *dead = *pp;
            *pp = dead->next;
            free(dead);
            return;
        }
        pp = &(*pp)->next;
    }
}

/* Insert one directed link and keep the matrix in sync. */
static int graph_add_directed(Graph *g, int src, int dst, double weight,
                              double bw_kbps, double delay_ms, double loss_rate,
                              int queue_cap)
{
    int id;
    Link *l;
    if (!graph_node_exists(g, src) || !graph_node_exists(g, dst) || src == dst) {
        return -1;
    }
    if (g->matrix_link[src][dst] >= 0) {
        return g->matrix_link[src][dst];
    }
    id = graph_alloc_link_slot(g);
    if (id < 0) {
        return -1;
    }
    l = &g->links[id];
    memset(l, 0, sizeof(*l));
    l->id = id;
    l->src = src;
    l->dst = dst;
    l->bandwidth_kbps = bw_kbps;
    l->prop_delay_ms = delay_ms;
    l->loss_rate = loss_rate;
    l->weight = weight;
    l->state = LINK_UP;
    l->queue_cap = queue_cap > 0 ? queue_cap : NETSIM_DEFAULT_QUEUE_K;
    l->tx_pkt = -1;
    l->busy_until = 0.0;
    l->reverse_id = -1;
    if (queue_init(&l->queue, l->queue_cap) != 0) {
        return -1;
    }
    if (graph_adj_insert(g, src, dst, id) != 0) {
        queue_free(&l->queue);
        g->link_used[id] = 0;
        return -1;
    }
    g->link_used[id] = 1;
    g->link_count += 1;
    g->matrix[src][dst] = weight;
    g->matrix_link[src][dst] = id;
    return id;
}

int graph_add_link(Graph *g, int src, int dst, double weight, double bw_kbps,
                   double delay_ms, double loss_rate, int queue_cap)
{
    int fwd;
    int rev;
    if (!g) {
        return -1;
    }
    fwd = graph_add_directed(g, src, dst, weight, bw_kbps, delay_ms, loss_rate,
                             queue_cap);
    if (fwd < 0) {
        return -1;
    }
    rev = graph_add_directed(g, dst, src, weight, bw_kbps, delay_ms, loss_rate,
                             queue_cap);
    if (rev < 0) {
        graph_remove_link(g, fwd);
        return -1;
    }
    g->links[fwd].reverse_id = rev;
    g->links[rev].reverse_id = fwd;
    return fwd;
}

/* Tear down one directed link without touching its reverse. */
static void graph_unlink_directed(Graph *g, int link_id)
{
    Link *l;
    if (!graph_link_exists(g, link_id)) {
        return;
    }
    l = &g->links[link_id];
    queue_free(&l->queue);
    graph_adj_remove(g, l->src, l->dst, link_id);
    g->matrix[l->src][l->dst] = NETSIM_INF;
    g->matrix_link[l->src][l->dst] = -1;
    g->link_used[link_id] = 0;
    g->link_count -= 1;
}

int graph_remove_link(Graph *g, int link_id)
{
    int rev;
    if (!graph_link_exists(g, link_id)) {
        return -1;
    }
    rev = g->links[link_id].reverse_id;
    graph_unlink_directed(g, link_id);
    if (rev >= 0 && g->link_used[rev]) {
        g->links[rev].reverse_id = -1;
        graph_unlink_directed(g, rev);
    }
    return 0;
}

int graph_remove_node(Graph *g, int node_id)
{
    int i;
    if (!graph_node_exists(g, node_id)) {
        return -1;
    }
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        if (g->link_used[i] &&
            (g->links[i].src == node_id || g->links[i].dst == node_id)) {
            graph_remove_link(g, i);
        }
    }
    g->node_used[node_id] = 0;
    g->node_count -= 1;
    graph_reset_matrix_row(g, node_id);
    return 0;
}

int graph_set_weight(Graph *g, int link_id, double weight)
{
    Link *l;
    Link *rev;
    if (!graph_link_exists(g, link_id) || weight < 0.0) {
        return -1;
    }
    l = &g->links[link_id];
    l->weight = weight;
    g->matrix[l->src][l->dst] = weight;
    if (l->reverse_id >= 0 && graph_link_exists(g, l->reverse_id)) {
        rev = &g->links[l->reverse_id];
        rev->weight = weight;
        g->matrix[rev->src][rev->dst] = weight;
    }
    return 0;
}

int graph_set_link_state(Graph *g, int link_id, LinkState state)
{
    Link *l;
    if (!graph_link_exists(g, link_id)) {
        return -1;
    }
    l = &g->links[link_id];
    l->state = state;
    if (l->reverse_id >= 0 && graph_link_exists(g, l->reverse_id)) {
        g->links[l->reverse_id].state = state;
    }
    return 0;
}

int graph_set_node_state(Graph *g, int node_id, NodeState state)
{
    if (!graph_node_exists(g, node_id)) {
        return -1;
    }
    g->nodes[node_id].state = state;
    return 0;
}

void graph_sync_matrix(Graph *g)
{
    int i;
    int u;
    if (!g) {
        return;
    }
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        graph_reset_matrix_row(g, i);
    }
    for (u = 0; u < NETSIM_MAX_NODES; u++) {
        if (g->node_used[u]) {
            g->matrix[u][u] = 0.0;
        }
    }
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        if (g->link_used[i]) {
            const Link *l = &g->links[i];
            g->matrix[l->src][l->dst] = l->weight;
            g->matrix_link[l->src][l->dst] = l->id;
        }
    }
}

/* Copy numeric parameters onto a single directed link. */
static int graph_apply_params(Link *l, double weight, double bw_kbps, double delay_ms,
                              double loss_rate, int queue_cap)
{
    l->weight = weight;
    l->bandwidth_kbps = bw_kbps;
    l->prop_delay_ms = delay_ms;
    l->loss_rate = loss_rate;
    if (queue_cap <= 0) {
        queue_cap = NETSIM_DEFAULT_QUEUE_K;
    }
    if (queue_cap != l->queue_cap) {
        if (queue_resize(&l->queue, queue_cap) != 0) {
            return -1;
        }
        l->queue_cap = queue_cap;
    }
    return 0;
}

int graph_set_link_params(Graph *g, int link_id, double weight, double bw_kbps,
                          double delay_ms, double loss_rate, int queue_cap)
{
    Link *l;
    if (!graph_link_exists(g, link_id) || weight < 0.0 || bw_kbps <= 0.0 ||
        delay_ms < 0.0 || loss_rate < 0.0 || loss_rate > 1.0) {
        return -1;
    }
    l = &g->links[link_id];
    if (graph_apply_params(l, weight, bw_kbps, delay_ms, loss_rate, queue_cap) != 0) {
        return -1;
    }
    g->matrix[l->src][l->dst] = weight;
    if (l->reverse_id >= 0 && graph_link_exists(g, l->reverse_id)) {
        Link *rev = &g->links[l->reverse_id];
        if (graph_apply_params(rev, weight, bw_kbps, delay_ms, loss_rate, queue_cap) !=
            0) {
            return -1;
        }
        g->matrix[rev->src][rev->dst] = weight;
    }
    return 0;
}
