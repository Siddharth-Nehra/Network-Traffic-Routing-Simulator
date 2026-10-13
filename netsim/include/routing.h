#ifndef NETSIM_ROUTING_H
#define NETSIM_ROUTING_H

#include "graph.h"

typedef struct RoutingResult {
    int src;
    int dst;
    double cost;
    int path[NETSIM_MAX_PATH];
    int path_len;
    int next_hop[NETSIM_MAX_NODES];
    double dist[NETSIM_MAX_NODES];
    int prev[NETSIM_MAX_NODES];
    int settled;
    int iterations;
    int negative_cycle;
    long runtime_us;
    RoutingAlgo algo;
} RoutingResult;

/* Edge cost used by the routing engine for the selected mode. */
double routing_edge_cost(const Graph *g, int link_id, CostMode mode);

/* Run Dijkstra from src using a binary min-heap. Fills distances and next hops. */
int routing_dijkstra(const Graph *g, int src, CostMode mode, RoutingResult *out);

/* Run Bellman-Ford from src. Detects negative cycles and counts iterations. */
int routing_bellman_ford(const Graph *g, int src, CostMode mode, RoutingResult *out);

/* Reconstruct src->dst path into out after a successful single-source run. */
int routing_build_path(RoutingResult *out, int dst);

/* Convenience: shortest path from src to dst with Dijkstra. */
int routing_shortest_path(const Graph *g, int src, int dst, CostMode mode,
                          RoutingAlgo algo, RoutingResult *out);

/* Format a path as "0 -> 1 -> 2" into buf. */
void routing_format_path(const RoutingResult *r, char *buf, size_t buflen);

#endif /* NETSIM_ROUTING_H */
