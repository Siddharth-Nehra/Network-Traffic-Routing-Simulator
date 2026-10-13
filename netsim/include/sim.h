#ifndef NETSIM_SIM_H
#define NETSIM_SIM_H

#include "events.h"
#include "graph.h"
#include "packet.h"
#include "routing.h"
#include "telemetry.h"
#include "traffic.h"

#define UI_FOC_NONE 0
#define UI_FOC_W 1
#define UI_FOC_BW 2
#define UI_FOC_DLY 3
#define UI_FOC_LOSS 4
#define UI_FOC_K 5
#define UI_FOC_FSRC 6
#define UI_FOC_FDST 7
#define UI_FOC_FRATE 8
#define UI_FOC_FSIZE 9
#define UI_FOC_FSTART 10
#define UI_FOC_FSTOP 11

typedef struct DropFlash {
    float x;
    float y;
    double until;
    int used;
} DropFlash;

typedef struct UiState {
    int focus;
    int flow_type;
    int compare_on;
    int active_tab;
    char in_weight[32];
    char in_bw[32];
    char in_delay[32];
    char in_loss[32];
    char in_k[32];
    char in_fsrc[32];
    char in_fdst[32];
    char in_frate[32];
    char in_fsize[32];
    char in_fstart[32];
    char in_fstop[32];
    char scenario[64];
    char topo_path[128];
    char log[16][128];
    int log_n;
} UiState;

typedef struct Simulator {
    Graph graph;
    RoutingResult route;
    RoutingResult cmp_dij;
    RoutingResult cmp_bf;
    RoutingAlgo algo;
    CostMode cost_mode;
    int next_hop[NETSIM_MAX_NODES][NETSIM_MAX_NODES];
    double rt_cost[NETSIM_MAX_NODES][NETSIM_MAX_NODES];
    PacketPool pool;
    EventSched sched;
    Flow flows[NETSIM_MAX_FLOWS];
    int flow_count;
    Telemetry tel;
    UiState ui;
    DropFlash flashes[NETSIM_MAX_FLASH];
    int path_src;
    int path_dst;
    int selected_node;
    int selected_link;
    int hover_node;
    int hover_link;
    int dragging;
    int drag_node;
    float drag_off_x;
    float drag_off_y;
    uint32_t seed;
    uint32_t rng_state;
    int screenshot_n;
    int window_w;
    int window_h;
    int should_close;
    int playing;
    int paused;
    double now;
    double end_time;
    double speed;
    int sample_armed;
    char status[256];
    char last_error[256];
} Simulator;

/* Allocate and initialize a simulator context. */
Simulator *sim_create(uint32_t seed);

/* Free the simulator and its graph storage. */
void sim_destroy(Simulator *sim);

/* Clear packets, events, flows, clock; keep the topology. */
void sim_reset_runtime(Simulator *sim);

/* Recompute tables, highlight path, and Dijkstra vs Bellman-Ford compare. */
int sim_recompute_route(Simulator *sim);

/* Build a 4-node square used by the headless demo. */
int sim_load_demo4(Simulator *sim);

/* xorshift32 in [0, 1). */
double sim_rand01(Simulator *sim);

/* Exponential random variable with the given mean. */
double sim_rand_exp(Simulator *sim, double mean);

/* Advance the DES clock by up to dt seconds. */
void sim_advance(Simulator *sim, double dt);

/* Process a single event (Step). */
int sim_step(Simulator *sim);

/* Start, pause, or resume the clock. */
void sim_play(Simulator *sim);
void sim_pause(Simulator *sim);

/* Emit a data or ACK packet from a flow. Returns packet index. */
int sim_emit_packet(Simulator *sim, int flow_id, int is_ack, int seq, int size);

/* Record a visual drop flash. */
void sim_flash_drop(Simulator *sim, float x, float y);

/* Toggle link or node failure and reroute. */
void sim_fail_selected(Simulator *sim);
void sim_on_topology_failure(Simulator *sim, int link_id);

/* Apply the right-panel link fields to the selected link. */
int sim_apply_link_fields(Simulator *sim);

/* Create a flow from the right-panel form. */
int sim_apply_flow_form(Simulator *sim);

/* Append a short log line for the UI. */
void sim_log(Simulator *sim, const char *fmt, ...);

#endif /* NETSIM_SIM_H */
