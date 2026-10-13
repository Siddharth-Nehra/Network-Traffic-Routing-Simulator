#include "scenarios.h"
#include "io.h"
#include "sim.h"

void scenario_list(void)
{
    printf("scenarios: demo4, normal_traffic, heavy_traffic, link_failure, "
           "high_loss, algorithm_compare, tcp_vs_udp, stress\n");
}

/* Clear flows but keep (or rebuild) topology. */
static void scenario_begin(Simulator *sim)
{
    memset(sim->flows, 0, sizeof(sim->flows));
    sim->flow_count = 0;
    packet_pool_init(&sim->pool);
    events_clear(&sim->sched);
    telemetry_init(&sim->tel);
    sim->now = 0.0;
    sim->playing = 0;
    sim->paused = 1;
    sim->sample_armed = 0;
    sim->end_time = 12.0;
}

static int scenario_square(Simulator *sim, double bw, double loss, int k)
{
    int n0, n1, n2, n3;
    graph_free(&sim->graph);
    graph_init(&sim->graph);
    n0 = graph_add_node(&sim->graph, 260, 180);
    n1 = graph_add_node(&sim->graph, 700, 180);
    n2 = graph_add_node(&sim->graph, 700, 480);
    n3 = graph_add_node(&sim->graph, 260, 480);
    if (n0 < 0 || n1 < 0 || n2 < 0 || n3 < 0) {
        return -1;
    }
    if (graph_add_link(&sim->graph, n0, n1, 1, bw, 10, loss, k) < 0 ||
        graph_add_link(&sim->graph, n1, n2, 1, bw, 10, loss, k) < 0 ||
        graph_add_link(&sim->graph, n2, n3, 4, bw, 10, loss, k) < 0 ||
        graph_add_link(&sim->graph, n3, n0, 1, bw, 10, loss, k) < 0 ||
        graph_add_link(&sim->graph, n0, n2, 10, bw * 0.5, 10, loss, k) < 0) {
        return -1;
    }
    sim->path_src = n0;
    sim->path_dst = n2;
    return 0;
}

static int load_normal(Simulator *sim)
{
    scenario_begin(sim);
    if (scenario_square(sim, 1000.0, 0.0, 50) != 0) {
        return -1;
    }
    sim->end_time = 8.0;
    if (traffic_add_flow(sim, 0, 2, FLOW_POISSON, 80.0, 500, 0.0, 7.5) < 0) {
        return -1;
    }
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "normal_traffic");
    return sim_recompute_route(sim);
}

static int load_heavy(Simulator *sim)
{
    scenario_begin(sim);
    if (scenario_square(sim, 200.0, 0.0, 8) != 0) {
        return -1;
    }
    sim->end_time = 8.0;
    if (traffic_add_flow(sim, 0, 2, FLOW_CBR, 150.0, 500, 0.0, 7.5) < 0 ||
        traffic_add_flow(sim, 0, 2, FLOW_CBR, 150.0, 500, 0.0, 7.5) < 0 ||
        traffic_add_flow(sim, 3, 1, FLOW_BURSTY, 120.0, 500, 0.0, 7.5) < 0) {
        return -1;
    }
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "heavy_traffic");
    return sim_recompute_route(sim);
}

static int load_failure(Simulator *sim)
{
    int link;
    scenario_begin(sim);
    if (scenario_square(sim, 800.0, 0.0, 30) != 0) {
        return -1;
    }
    sim->end_time = 10.0;
    if (traffic_add_flow(sim, 0, 2, FLOW_CBR, 120.0, 500, 0.0, 9.5) < 0) {
        return -1;
    }
    link = graph_find_link(&sim->graph, 0, 1);
    events_schedule(&sim->sched, 4.0, EV_LINK_FAIL, link, 0, 0, 0.0);
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "link_failure");
    return sim_recompute_route(sim);
}

static int load_loss(Simulator *sim)
{
    scenario_begin(sim);
    if (scenario_square(sim, 1000.0, 0.2, 40) != 0) {
        return -1;
    }
    sim->end_time = 8.0;
    if (traffic_add_flow(sim, 0, 2, FLOW_CBR, 100.0, 500, 0.0, 7.5) < 0) {
        return -1;
    }
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "high_loss");
    return sim_recompute_route(sim);
}

static int load_compare(Simulator *sim)
{
    scenario_begin(sim);
    if (io_load_topo(sim, "topologies/mesh8.topo") != 0) {
        if (scenario_square(sim, 1000.0, 0.0, 20) != 0) {
            return -1;
        }
    }
    if (sim->path_dst < 0) {
        sim->path_dst = 2;
    }
    sim->path_src = 0;
    sim->end_time = 1.0;
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "algorithm_compare");
    return sim_recompute_route(sim);
}

static int load_tcp_udp(Simulator *sim)
{
    scenario_begin(sim);
    if (scenario_square(sim, 250.0, 0.0, 15) != 0) {
        return -1;
    }
    sim->end_time = 8.0;
    if (traffic_add_flow(sim, 0, 2, FLOW_TCP, 400.0, 500, 0.0, 7.5) < 0 ||
        traffic_add_flow(sim, 0, 2, FLOW_CBR, 180.0, 500, 0.0, 7.5) < 0) {
        return -1;
    }
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "tcp_vs_udp");
    return sim_recompute_route(sim);
}

static int load_stress(Simulator *sim)
{
    int r, c, id;
    int cols = 10;
    int rows = 5;
    scenario_begin(sim);
    graph_free(&sim->graph);
    graph_init(&sim->graph);
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            id = graph_add_node(&sim->graph, 80.0f + (float)c * 70.0f,
                                80.0f + (float)r * 80.0f);
            if (id < 0) {
                return -1;
            }
            if (c > 0 &&
                graph_add_link(&sim->graph, id - 1, id, 1, 500, 5, 0, 16) < 0) {
                return -1;
            }
            if (r > 0 &&
                graph_add_link(&sim->graph, id - cols, id, 1, 500, 5, 0, 16) < 0) {
                return -1;
            }
        }
    }
    sim->path_src = 0;
    sim->path_dst = rows * cols - 1;
    sim->end_time = 3.0;
    for (id = 0; id < 12; id++) {
        int s = id * 3;
        int d = 49 - id;
        if (s == d) {
            d = 48;
        }
        if (traffic_add_flow(sim, s, d, FLOW_CBR, 40.0, 400, 0.0, 2.8) < 0) {
            return -1;
        }
    }
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "stress");
    return sim_recompute_route(sim);
}

static int load_demo(Simulator *sim)
{
    scenario_begin(sim);
    snprintf(sim->ui.scenario, sizeof(sim->ui.scenario), "demo4");
    sim->end_time = 1.0;
    return sim_load_demo4(sim);
}

int scenario_load(Simulator *sim, const char *name)
{
    if (!sim || !name) {
        return -1;
    }
    if (strcmp(name, "demo4") == 0) {
        return load_demo(sim);
    }
    if (strcmp(name, "normal_traffic") == 0) {
        return load_normal(sim);
    }
    if (strcmp(name, "heavy_traffic") == 0) {
        return load_heavy(sim);
    }
    if (strcmp(name, "link_failure") == 0) {
        return load_failure(sim);
    }
    if (strcmp(name, "high_loss") == 0) {
        return load_loss(sim);
    }
    if (strcmp(name, "algorithm_compare") == 0) {
        return load_compare(sim);
    }
    if (strcmp(name, "tcp_vs_udp") == 0) {
        return load_tcp_udp(sim);
    }
    if (strcmp(name, "stress") == 0) {
        return load_stress(sim);
    }
    return -2;
}

int scenario_run_headless(Simulator *sim, const char *name, const char *csv)
{
    int rc = scenario_load(sim, name);
    if (rc == -2) {
        fprintf(stderr, "unknown scenario '%s'\n", name);
        scenario_list();
        return 2;
    }
    if (rc != 0) {
        fprintf(stderr, "failed to load scenario '%s'\n", name);
        return 1;
    }
    sim_play(sim);
    for (;;) {
        double nt;
        if (!events_peek_time(&sim->sched, &nt)) {
            break;
        }
        if (nt > sim->end_time + 1e-9) {
            break;
        }
        if (!sim_step(sim)) {
            break;
        }
    }
    telemetry_sample(sim);
    telemetry_print_summary(sim, name);
    if (csv && io_export_csv(sim, csv, name) != 0) {
        return 1;
    }
    return 0;
}
