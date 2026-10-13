#include "events.h"
#include "graph.h"
#include "heap.h"
#include "queue.h"
#include "routing.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_heap_order(void)
{
    Heap h;
    HeapItem a, b, c;
    assert(heap_init(&h) == 0);
    assert(heap_push(&h, 5.0, 5, NULL) == 0);
    assert(heap_push(&h, 1.0, 1, NULL) == 0);
    assert(heap_push(&h, 3.0, 3, NULL) == 0);
    assert(heap_pop(&h, &a) == 0 && a.value == 1);
    assert(heap_pop(&h, &b) == 0 && b.value == 3);
    assert(heap_pop(&h, &c) == 0 && c.value == 5);
    heap_free(&h);
}

static void test_graph_list_matrix_sync(void)
{
    Graph g;
    int a, b, link;
    graph_init(&g);
    a = graph_add_node(&g, 0, 0);
    b = graph_add_node(&g, 1, 0);
    link = graph_add_link(&g, a, b, 2.5, 1000, 10, 0, 8);
    assert(link >= 0);
    assert(g.matrix[a][b] == 2.5);
    assert(g.matrix[b][a] == 2.5);
    assert(queue_len(&g.links[link].queue) == 0);
    assert(graph_set_weight(&g, link, 7.0) == 0);
    assert(g.matrix[a][b] == 7.0);
    graph_sync_matrix(&g);
    assert(g.matrix[b][a] == 7.0);
    assert(graph_remove_link(&g, link) == 0);
    assert(g.matrix[a][b] >= NETSIM_INF * 0.5);
    graph_free(&g);
}

static void build_square(Graph *g)
{
    int n0, n1, n2, n3;
    graph_init(g);
    n0 = graph_add_node(g, 0, 0);
    n1 = graph_add_node(g, 1, 0);
    n2 = graph_add_node(g, 1, 1);
    n3 = graph_add_node(g, 0, 1);
    assert(n0 == 0 && n1 == 1 && n2 == 2 && n3 == 3);
    assert(graph_add_link(g, n0, n1, 1, 1000, 10, 0, 8) >= 0);
    assert(graph_add_link(g, n1, n2, 1, 1000, 10, 0, 8) >= 0);
    assert(graph_add_link(g, n2, n3, 4, 1000, 10, 0, 8) >= 0);
    assert(graph_add_link(g, n3, n0, 1, 1000, 10, 0, 8) >= 0);
    assert(graph_add_link(g, n0, n2, 10, 1000, 10, 0, 8) >= 0);
}

static void test_dijkstra_square(void)
{
    Graph g;
    RoutingResult r;
    build_square(&g);
    assert(routing_shortest_path(&g, 0, 2, COST_STATIC_WEIGHT, ROUTING_DIJKSTRA, &r) == 0);
    assert(r.path_len == 3);
    assert(fabs(r.cost - 2.0) < 1e-9);
    graph_set_link_state(&g, graph_find_link(&g, 0, 1), LINK_DOWN);
    assert(routing_shortest_path(&g, 0, 2, COST_STATIC_WEIGHT, ROUTING_DIJKSTRA, &r) == 0);
    assert(fabs(r.cost - 5.0) < 1e-9);
    graph_free(&g);
}

static void test_bellman_matches_dijkstra(void)
{
    Graph g;
    RoutingResult d;
    RoutingResult b;
    int i;
    build_square(&g);
    assert(routing_dijkstra(&g, 0, COST_STATIC_WEIGHT, &d) == 0);
    assert(routing_bellman_ford(&g, 0, COST_STATIC_WEIGHT, &b) == 0);
    assert(!d.negative_cycle && !b.negative_cycle);
    for (i = 0; i < 4; i++) {
        assert(fabs(d.dist[i] - b.dist[i]) < 1e-9);
        assert(d.next_hop[i] == b.next_hop[i]);
    }
    assert(routing_build_path(&d, 2) == 0);
    assert(routing_build_path(&b, 2) == 0);
    assert(d.path_len == b.path_len);
    assert(b.iterations >= 1);
    graph_free(&g);
}

static void test_queue_tail_drop(void)
{
    PacketQueue q;
    int v;
    assert(queue_init(&q, 2) == 0);
    assert(queue_push(&q, 10) == 0);
    assert(queue_push(&q, 11) == 0);
    assert(queue_push(&q, 12) == -1);
    assert(queue_full(&q));
    assert(queue_pop(&q, &v) == 0 && v == 10);
    assert(queue_pop(&q, &v) == 0 && v == 11);
    assert(queue_pop(&q, &v) == -1);
    queue_free(&q);
}

static void test_events_order(void)
{
    EventSched es;
    SimEvent a, b, c;
    assert(events_init(&es) == 0);
    assert(events_schedule(&es, 5.0, EV_END, 0, 0, 0, 0) >= 0);
    assert(events_schedule(&es, 1.0, EV_FLOW_SEND, 1, 0, 0, 0) >= 0);
    assert(events_schedule(&es, 3.0, EV_SAMPLE, 0, 0, 0, 0) >= 0);
    assert(events_pop(&es, &a) == 0 && a.time == 1.0 && a.type == EV_FLOW_SEND);
    assert(events_pop(&es, &b) == 0 && b.time == 3.0);
    assert(events_pop(&es, &c) == 0 && c.time == 5.0 && c.type == EV_END);
    events_free(&es);
}

int main(void)
{
    test_heap_order();
    test_graph_list_matrix_sync();
    test_dijkstra_square();
    test_bellman_matches_dijkstra();
    test_queue_tail_drop();
    test_events_order();
    printf("all tests passed\n");
    return 0;
}
