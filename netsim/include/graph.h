#ifndef NETSIM_GRAPH_H
#define NETSIM_GRAPH_H

#include "common.h"
#include "queue.h"

typedef struct Node {
    int id;
    char name[NETSIM_MAX_NAME];
    float x;
    float y;
    NodeState state;
    int packets_forwarded;
    int packets_dropped;
} Node;

typedef struct Link {
    int id;
    int src;
    int dst;
    double bandwidth_kbps;
    double prop_delay_ms;
    double loss_rate;
    double weight;
    LinkState state;
    int queue_cap;
    PacketQueue queue;
    int tx_pkt;
    double busy_until;
    uint64_t bytes_sent;
    double qdelay_sum;
    int qdelay_n;
    double occupancy_sum;
    int occupancy_n;
    double utilization;
    int reverse_id;
} Link;

typedef struct AdjListNode {
    int neighbor;
    int link_id;
    struct AdjListNode *next;
} AdjListNode;

typedef struct Graph {
    int node_count;
    int link_count;
    Node nodes[NETSIM_MAX_NODES];
    int node_used[NETSIM_MAX_NODES];
    Link links[NETSIM_MAX_LINKS];
    int link_used[NETSIM_MAX_LINKS];
    AdjListNode *adj[NETSIM_MAX_NODES];
    double matrix[NETSIM_MAX_NODES][NETSIM_MAX_NODES];
    int matrix_link[NETSIM_MAX_NODES][NETSIM_MAX_NODES];
} Graph;

/* Clear the graph and set matrix cells to infinity. */
void graph_init(Graph *g);

/* Free adjacency-list nodes. */
void graph_free(Graph *g);

/* Add a node at canvas position (x, y). Returns id or -1. */
int graph_add_node(Graph *g, float x, float y);

/* Remove a node and all incident links. Returns 0 on success. */
int graph_remove_node(Graph *g, int node_id);

/* Add a bidirectional link (two directed edges). Returns forward link id. */
int graph_add_link(Graph *g, int src, int dst, double weight, double bw_kbps,
                   double delay_ms, double loss_rate, int queue_cap);

/* Remove both directions of a link pair. Returns 0 on success. */
int graph_remove_link(Graph *g, int link_id);

/* Update static weight on both directions of a link pair. */
int graph_set_weight(Graph *g, int link_id, double weight);

/* Fail or restore a directed link (and its reverse). */
int graph_set_link_state(Graph *g, int link_id, LinkState state);

/* Fail or restore a node. */
int graph_set_node_state(Graph *g, int node_id, NodeState state);

/* Return 1 if the node slot is in use. */
int graph_node_exists(const Graph *g, int node_id);

/* Return 1 if the link slot is in use. */
int graph_link_exists(const Graph *g, int link_id);

/* Find directed link id from src to dst, or -1. */
int graph_find_link(const Graph *g, int src, int dst);

/* Return a used node/link pointer, or NULL. */
Node *graph_node(Graph *g, int node_id);
const Node *graph_node_const(const Graph *g, int node_id);
Link *graph_link(Graph *g, int link_id);
const Link *graph_link_const(const Graph *g, int link_id);

/* Rebuild adjacency matrix from the current directed links. */
void graph_sync_matrix(Graph *g);

/* Apply editable link parameters to both directions. */
int graph_set_link_params(Graph *g, int link_id, double weight, double bw_kbps,
                          double delay_ms, double loss_rate, int queue_cap);

#endif /* NETSIM_GRAPH_H */
