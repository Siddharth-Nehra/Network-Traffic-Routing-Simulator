#include "ui_panels.h"
#include "io.h"
#include "scenarios.h"
#include "sim.h"
#include "ui_canvas.h"
#include "ui_fonts.h"

#include "raylib.h"
#include <stdio.h>
#include <string.h>

static int in_rect(Rectangle r, Vector2 p)
{
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height;
}

static void load_scenario_named(Simulator *sim, const char *name)
{
    if (scenario_load(sim, name) == 0) {
        sim_log(sim, "loaded %s", name);
    } else {
        sim_log(sim, "failed %s", name);
    }
}

static void draw_top_bar(Simulator *sim, int w)
{
    Rectangle rstart = {350, 11, 80, 32};
    Rectangle rstep  = {438, 11, 75, 32};
    Rectangle rreset = {521, 11, 75, 32};
    Rectangle sl     = {665, 22, 100, 10};
    Rectangle rhint  = {(float)w - 400, 14, 275, 26};
    Rectangle rsnap  = {(float)w - 115, 12, 100, 30};
    Vector2 m = GetMousePosition();
    char spd_buf[16];

    /* Bar background */
    DrawRectangle(0, 0, w, 54, (Color){14, 18, 28, 255});
    DrawLine(0, 53, w, 53, (Color){36, 45, 66, 255});

    /* Left Branding: Logo badge + Title */
    ui_draw_badge(14, 13, "NETSIM", (Color){37, 99, 235, 255}, RAYWHITE);
    ui_draw_text_bold("Routing & Traffic Simulator", 85, 17, 15.0f, RAYWHITE);

    /* Playback controls */
    if (sim->playing) {
        if (ui_button(rstart, "Pause", (Color){217, 119, 6, 255}, RAYWHITE, 1)) {
            sim_pause(sim);
        }
    } else {
        if (ui_button(rstart, "Start", (Color){16, 185, 129, 255}, RAYWHITE, 0)) {
            sim_play(sim);
        }
    }

    if (ui_button(rstep, "Step", (Color){37, 99, 235, 255}, RAYWHITE, 0)) {
        sim_step(sim);
    }
    if (ui_button(rreset, "Reset", (Color){185, 28, 28, 255}, RAYWHITE, 0)) {
        sim_reset_runtime(sim);
    }

    /* Speed slider */
    ui_draw_text_sm("Speed", 615, 20, 13.0f, (Color){160, 175, 200, 255});
    DrawRectangleRounded(sl, 0.5f, 4, (Color){28, 36, 52, 255});
    DrawRectangleRoundedLines(sl, 0.5f, 4, (Color){45, 56, 82, 255});

    {
        float t_val = (float)((sim->speed - 0.1) / 9.9);
        if (t_val < 0.0f) t_val = 0.0f;
        if (t_val > 1.0f) t_val = 1.0f;

        /* Active track fill */
        DrawRectangleRounded((Rectangle){sl.x, sl.y, sl.width * t_val, sl.height}, 0.5f, 4, (Color){59, 130, 246, 255});

        /* Thumb */
        DrawCircle((int)(sl.x + t_val * sl.width), (int)(sl.y + sl.height * 0.5f), 7.0f, RAYWHITE);
        DrawCircle((int)(sl.x + t_val * sl.width), (int)(sl.y + sl.height * 0.5f), 4.0f, (Color){37, 99, 235, 255});

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && in_rect((Rectangle){sl.x - 5, sl.y - 5, sl.width + 10, sl.height + 10}, m)) {
            double nt = (m.x - sl.x) / sl.width;
            if (nt < 0.0) nt = 0.0;
            if (nt > 1.0) nt = 1.0;
            sim->speed = 0.1 + nt * 9.9;
        }
    }

    snprintf(spd_buf, sizeof(spd_buf), "%.1fx", sim->speed);
    ui_draw_pill((Rectangle){775, 16, 50, 22}, spd_buf, (Color){24, 32, 48, 255}, (Color){96, 165, 250, 255}, 1);

    /* Shortcut hint pill */
    ui_draw_pill(rhint, "Space: Run | F: Fail Link | Del: Delete",
                 (Color){20, 26, 40, 255}, (Color){148, 163, 184, 255}, 0);

    /* Snapshot button */
    if (ui_button(rsnap, "Snapshot", (Color){30, 41, 59, 255}, (Color){203, 213, 225, 255}, 0)) {
        char path[160];
        snprintf(path, sizeof(path), "docs/screenshots/shot_%d.png", sim->screenshot_n);
        TakeScreenshot(path);
        sim_log(sim, "saved %s", path);
        sim->screenshot_n += 1;
    }
}

static void draw_tab_controls(Simulator *sim, int x, int y, int pw)
{
    Rectangle c1 = {(float)x, (float)y, (float)pw, 126};
    Rectangle c2 = {(float)x, (float)y + 134, (float)pw, 100};
    Rectangle c3 = {(float)x, (float)y + 242, (float)pw, 140};
    Rectangle c4 = {(float)x, (float)y + 390, (float)pw, 100};

    /* Card 1: Routing Algorithm */
    ui_draw_card(c1, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Routing Algorithm", x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});

    if (ui_button((Rectangle){(float)x + 12, (float)y + 32, (float)(pw - 30) * 0.5f, 30},
                  "Dijkstra (SPF)",
                  sim->algo == ROUTING_DIJKSTRA ? (Color){37, 99, 235, 255} : (Color){28, 35, 50, 255},
                  RAYWHITE, sim->algo == ROUTING_DIJKSTRA)) {
        sim->algo = ROUTING_DIJKSTRA;
        sim_recompute_route(sim);
    }

    if (ui_button((Rectangle){(float)x + 18 + (float)(pw - 30) * 0.5f, (float)y + 32, (float)(pw - 30) * 0.5f, 30},
                  "Bellman-Ford",
                  sim->algo == ROUTING_BELLMAN_FORD ? (Color){37, 99, 235, 255} : (Color){28, 35, 50, 255},
                  RAYWHITE, sim->algo == ROUTING_BELLMAN_FORD)) {
        sim->algo = ROUTING_BELLMAN_FORD;
        sim_recompute_route(sim);
    }

    if (sim->algo == ROUTING_DIJKSTRA) {
        ui_draw_text_sm("O(E log V) link-state algorithm. Finds exact shortest", x + 14, y + 74, 11.0f, (Color){148, 163, 184, 255});
        ui_draw_text_sm("paths using priority queue. Ideal for OSPF / IS-IS.", x + 14, y + 92, 11.0f, (Color){148, 163, 184, 255});
    } else {
        ui_draw_text_sm("O(V*E) distance-vector algorithm. Iterative relaxation", x + 14, y + 74, 11.0f, (Color){148, 163, 184, 255});
        ui_draw_text_sm("models RIP routing protocol. Detects negative cycles.", x + 14, y + 92, 11.0f, (Color){148, 163, 184, 255});
    }

    /* Card 2: Cost Metric Mode */
    ui_draw_card(c2, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Cost Metric Mode", x + 12, y + 144, 13.0f, (Color){226, 232, 240, 255});

    {
        float bw = (float)(pw - 36) / 3.0f;
        if (ui_button((Rectangle){(float)x + 12, (float)y + 168, bw, 28}, "Weight",
                      sim->cost_mode == COST_STATIC_WEIGHT ? (Color){37, 99, 235, 255} : (Color){28, 35, 50, 255},
                      RAYWHITE, sim->cost_mode == COST_STATIC_WEIGHT)) {
            sim->cost_mode = COST_STATIC_WEIGHT;
            sim_recompute_route(sim);
        }
        if (ui_button((Rectangle){(float)x + 18 + bw, (float)y + 168, bw, 28}, "Hop Count",
                      sim->cost_mode == COST_HOP_COUNT ? (Color){37, 99, 235, 255} : (Color){28, 35, 50, 255},
                      RAYWHITE, sim->cost_mode == COST_HOP_COUNT)) {
            sim->cost_mode = COST_HOP_COUNT;
            sim_recompute_route(sim);
        }
        if (ui_button((Rectangle){(float)x + 24 + bw * 2.0f, (float)y + 168, bw, 28}, "Dynamic",
                      sim->cost_mode == COST_DYNAMIC ? (Color){37, 99, 235, 255} : (Color){28, 35, 50, 255},
                      RAYWHITE, sim->cost_mode == COST_DYNAMIC)) {
            sim->cost_mode = COST_DYNAMIC;
            sim_recompute_route(sim);
        }
    }
    ui_draw_text_sm(sim->cost_mode == COST_STATIC_WEIGHT ? "Routes based on link administrative weights."
                    : (sim->cost_mode == COST_HOP_COUNT ? "Minimizes number of traversed network hops."
                                                        : "Dynamically considers queue backlog and latency."),
                    x + 14, y + 204, 11.0f, (Color){148, 163, 184, 255});

    /* Card 3: Scenarios */
    ui_draw_card(c3, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Preset Scenarios", x + 12, y + 252, 13.0f, (Color){226, 232, 240, 255});

    {
        float sbw = (float)(pw - 36) / 3.0f;
        if (ui_button((Rectangle){(float)x + 12, (float)y + 276, sbw, 28}, "Normal", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "normal_traffic");
        }
        if (ui_button((Rectangle){(float)x + 18 + sbw, (float)y + 276, sbw, 28}, "Heavy", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "heavy_traffic");
        }
        if (ui_button((Rectangle){(float)x + 24 + sbw * 2.0f, (float)y + 276, sbw, 28}, "Failure", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "link_failure");
        }
        if (ui_button((Rectangle){(float)x + 12, (float)y + 312, sbw, 28}, "Loss", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "high_loss");
        }
        if (ui_button((Rectangle){(float)x + 18 + sbw, (float)y + 312, sbw, 28}, "Compare", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "algorithm_compare");
        }
        if (ui_button((Rectangle){(float)x + 24 + sbw * 2.0f, (float)y + 312, sbw, 28}, "TCP vs UDP", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            load_scenario_named(sim, "tcp_vs_udp");
        }
    }
    ui_draw_text_sm("Current scenario: ", x + 14, y + 352, 11.0f, (Color){148, 163, 184, 255});
    ui_draw_text_bold_sm(sim->ui.scenario[0] ? sim->ui.scenario : "custom", x + 120, y + 352, 11.0f, (Color){52, 211, 153, 255});

    /* Card 4: Topo & CSV */
    ui_draw_card(c4, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Topology & CSV Export", x + 12, y + 400, 13.0f, (Color){226, 232, 240, 255});

    {
        float tbw = (float)(pw - 36) / 3.0f;
        if (ui_button((Rectangle){(float)x + 12, (float)y + 424, tbw, 28}, "Save Topo", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            io_save_topo(sim, sim->ui.topo_path);
            sim_log(sim, "saved %s", sim->ui.topo_path);
        }
        if (ui_button((Rectangle){(float)x + 18 + tbw, (float)y + 424, tbw, 28}, "Load Topo", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            io_load_topo(sim, sim->ui.topo_path);
            sim_log(sim, "loaded topo");
        }
        if (ui_button((Rectangle){(float)x + 24 + tbw * 2.0f, (float)y + 424, tbw, 28}, "Export CSV", (Color){28, 36, 52, 255}, (Color){226, 232, 240, 255}, 0)) {
            io_export_csv(sim, "results_live.csv", sim->ui.scenario);
            sim_log(sim, "wrote results_live.csv");
        }
    }
    ui_draw_text_sm("File: topologies/session.topo | results_live.csv", x + 14, y + 460, 11.0f, (Color){148, 163, 184, 255});
}

static void draw_tab_elements(Simulator *sim, int x, int y, int pw)
{
    Rectangle c_link = {(float)x, (float)y, (float)pw, 210};
    Rectangle c_flow = {(float)x, (float)y + 220, (float)pw, 235};

    /* Card 1: Element Inspector (Link or Node) */
    ui_draw_card(c_link, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});

    if (sim->selected_link >= 0) {
        const Link *l = graph_link_const(&sim->graph, sim->selected_link);
        char link_title[64];
        float iw = (float)(pw - 48) / 5.0f;

        if (l) {
            snprintf(link_title, sizeof(link_title), "Link #%d (Node %d <-> %d)", sim->selected_link, l->src, l->dst);
        } else {
            snprintf(link_title, sizeof(link_title), "Link #%d", sim->selected_link);
        }

        ui_draw_text_bold(link_title, x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});
        if (l && l->state == LINK_UP) {
            ui_draw_badge(x + pw - 80, y + 8, "LINK UP", (Color){6, 78, 59, 255}, (Color){52, 211, 153, 255});
        } else {
            ui_draw_badge(x + pw - 95, y + 8, "LINK DOWN", (Color){127, 29, 29, 255}, (Color){248, 113, 113, 255});
        }

        /* Input fields with clear labels */
        ui_input(sim, (Rectangle){(float)x + 12, (float)y + 54, iw, 26}, sim->ui.in_weight, 32, UI_FOC_W, "Weight", "1.0");
        ui_input(sim, (Rectangle){(float)x + 18 + iw, (float)y + 54, iw, 26}, sim->ui.in_bw, 32, UI_FOC_BW, "BW kbps", "1000");
        ui_input(sim, (Rectangle){(float)x + 24 + iw * 2.0f, (float)y + 54, iw, 26}, sim->ui.in_delay, 32, UI_FOC_DLY, "Delay ms", "10");
        ui_input(sim, (Rectangle){(float)x + 30 + iw * 3.0f, (float)y + 54, iw, 26}, sim->ui.in_loss, 32, UI_FOC_LOSS, "Loss rate", "0.0");
        ui_input(sim, (Rectangle){(float)x + 36 + iw * 4.0f, (float)y + 54, iw, 26}, sim->ui.in_k, 32, UI_FOC_K, "Queue K", "20");

        /* Live link status info */
        if (l) {
            char q_info[64];
            snprintf(q_info, sizeof(q_info), "Queue: %d / %d pkts  |  Util: %.1f%%  |  Sent: %llu B",
                     queue_len(&l->queue), l->queue_cap, l->utilization * 100.0, (unsigned long long)l->bytes_sent);
            ui_draw_text_mono_sm(q_info, x + 14, y + 96, 11.0f, (Color){148, 163, 184, 255});
        }

        /* Buttons */
        if (ui_button((Rectangle){(float)x + 12, (float)y + 122, 130, 30}, "Apply Params",
                      (Color){16, 185, 129, 255}, RAYWHITE, 0)) {
            sim_apply_link_fields(sim);
        }

        if (ui_button((Rectangle){(float)x + 150, (float)y + 122, 140, 30}, "Fail / Restore",
                      (Color){220, 38, 38, 255}, RAYWHITE, 0)) {
            sim_fail_selected(sim);
        }

        ui_draw_text_sm("Edit parameter boxes and press Enter or Apply.", x + 14, y + 164, 11.0f, (Color){100, 116, 139, 255});
        ui_draw_text_sm("Press F to quickly toggle fail/restore.", x + 14, y + 182, 11.0f, (Color){100, 116, 139, 255});

    } else if (sim->selected_node >= 0) {
        const Node *n = graph_node_const(&sim->graph, sim->selected_node);
        char node_title[64];
        snprintf(node_title, sizeof(node_title), "Node #%d Inspector", sim->selected_node);
        ui_draw_text_bold(node_title, x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});

        if (n && n->state == NODE_UP) {
            ui_draw_badge(x + pw - 80, y + 8, "NODE UP", (Color){6, 78, 59, 255}, (Color){52, 211, 153, 255});
        } else {
            ui_draw_badge(x + pw - 95, y + 8, "NODE DOWN", (Color){127, 29, 29, 255}, (Color){248, 113, 113, 255});
        }

        ui_draw_text_sm("Node controls for routing path endpoints & failure test:", x + 14, y + 36, 11.0f, (Color){148, 163, 184, 255});

        if (ui_button((Rectangle){(float)x + 12, (float)y + 60, 110, 28}, "Set as SRC",
                      sim->path_src == sim->selected_node ? (Color){16, 185, 129, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->path_src == sim->selected_node)) {
            sim->path_src = sim->selected_node;
            sim_recompute_route(sim);
        }

        if (ui_button((Rectangle){(float)x + 130, (float)y + 60, 110, 28}, "Set as DST",
                      sim->path_dst == sim->selected_node ? (Color){249, 115, 22, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->path_dst == sim->selected_node)) {
            sim->path_dst = sim->selected_node;
            sim_recompute_route(sim);
        }

        if (ui_button((Rectangle){(float)x + 12, (float)y + 98, 130, 28}, "Toggle UP / DOWN",
                      (Color){220, 38, 38, 255}, RAYWHITE, 0)) {
            sim_fail_selected(sim);
        }

        if (ui_button((Rectangle){(float)x + 150, (float)y + 98, 110, 28}, "Delete Node",
                      (Color){127, 29, 29, 255}, RAYWHITE, 0)) {
            graph_remove_node(&sim->graph, sim->selected_node);
            sim->selected_node = -1;
            sim_recompute_route(sim);
        }

        ui_draw_text_sm("Tip: Click another node to connect a link between them.", x + 14, y + 140, 11.0f, (Color){100, 116, 139, 255});
        ui_draw_text_sm("Drag node to reposition on canvas.", x + 14, y + 160, 11.0f, (Color){100, 116, 139, 255});

    } else {
        ui_draw_text_bold("Element Inspector", x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});
        ui_draw_text_sm("No node or link selected.", x + 14, y + 40, 12.0f, (Color){148, 163, 184, 255});
        ui_draw_text_sm("* Click any node or link on canvas to inspect & configure.", x + 14, y + 70, 11.0f, (Color){100, 116, 139, 255});
        ui_draw_text_sm("* Click empty canvas area to add a new network node.", x + 14, y + 92, 11.0f, (Color){100, 116, 139, 255});
        ui_draw_text_sm("* Right-click nodes to quickly set Source and Destination.", x + 14, y + 114, 11.0f, (Color){100, 116, 139, 255});
        ui_draw_text_sm("* Press Space to run/pause, F to fail link, Del to delete.", x + 14, y + 136, 11.0f, (Color){100, 116, 139, 255});
    }

    /* Card 2: Traffic Flow Generator */
    ui_draw_card(c_flow, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Traffic Flow Generator", x + 12, y + 230, 13.0f, (Color){226, 232, 240, 255});

    {
        float fiw = (float)(pw - 52) / 6.0f;
        ui_input(sim, (Rectangle){(float)x + 12, (float)y + 272, fiw, 24}, sim->ui.in_fsrc, 32, UI_FOC_FSRC, "Src", "0");
        ui_input(sim, (Rectangle){(float)x + 18 + fiw, (float)y + 272, fiw, 24}, sim->ui.in_fdst, 32, UI_FOC_FDST, "Dst", "2");
        ui_input(sim, (Rectangle){(float)x + 24 + fiw * 2.0f, (float)y + 272, fiw, 24}, sim->ui.in_frate, 32, UI_FOC_FRATE, "Rate", "100");
        ui_input(sim, (Rectangle){(float)x + 30 + fiw * 3.0f, (float)y + 272, fiw, 24}, sim->ui.in_fsize, 32, UI_FOC_FSIZE, "Size", "500");
        ui_input(sim, (Rectangle){(float)x + 36 + fiw * 4.0f, (float)y + 272, fiw, 24}, sim->ui.in_fstart, 32, UI_FOC_FSTART, "Start", "0");
        ui_input(sim, (Rectangle){(float)x + 42 + fiw * 5.0f, (float)y + 272, fiw, 24}, sim->ui.in_fstop, 32, UI_FOC_FSTOP, "Stop", "10");
    }

    /* Flow Protocol Buttons */
    ui_draw_text_sm("Protocol Type:", x + 14, y + 312, 11.0f, (Color){148, 163, 184, 255});
    {
        float pbw = (float)(pw - 42) / 4.0f;
        if (ui_button((Rectangle){(float)x + 12, (float)y + 330, pbw, 26}, "CBR",
                      sim->ui.flow_type == FLOW_CBR ? (Color){37, 99, 235, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->ui.flow_type == FLOW_CBR)) {
            sim->ui.flow_type = FLOW_CBR;
        }
        if (ui_button((Rectangle){(float)x + 18 + pbw, (float)y + 330, pbw, 26}, "Bursty",
                      sim->ui.flow_type == FLOW_BURSTY ? (Color){37, 99, 235, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->ui.flow_type == FLOW_BURSTY)) {
            sim->ui.flow_type = FLOW_BURSTY;
        }
        if (ui_button((Rectangle){(float)x + 24 + pbw * 2.0f, (float)y + 330, pbw, 26}, "TCP",
                      sim->ui.flow_type == FLOW_TCP ? (Color){37, 99, 235, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->ui.flow_type == FLOW_TCP)) {
            sim->ui.flow_type = FLOW_TCP;
        }
        if (ui_button((Rectangle){(float)x + 30 + pbw * 3.0f, (float)y + 330, pbw, 26}, "Poisson",
                      sim->ui.flow_type == FLOW_POISSON ? (Color){37, 99, 235, 255} : (Color){28, 36, 52, 255},
                      RAYWHITE, sim->ui.flow_type == FLOW_POISSON)) {
            sim->ui.flow_type = FLOW_POISSON;
        }
    }

    /* Add Flow Button */
    if (ui_button((Rectangle){(float)x + 12, (float)y + 368, (float)pw - 24, 30}, "+ Inject Traffic Flow",
                  (Color){16, 185, 129, 255}, RAYWHITE, 0)) {
        sim_apply_flow_form(sim);
    }

    {
        char flow_sum[64];
        snprintf(flow_sum, sizeof(flow_sum), "Active Flows Configured: %d / %d", sim->flow_count, NETSIM_MAX_FLOWS);
        ui_draw_text_mono_sm(flow_sum, x + 14, y + 412, 11.0f, (Color){148, 163, 184, 255});
    }
}

static void draw_tab_routing(Simulator *sim, int x, int y, int pw)
{
    Rectangle c_cmp = {(float)x, (float)y, (float)pw, 150};
    Rectangle c_adj = {(float)x, (float)y + 158, (float)pw, 150};
    Rectangle c_mat = {(float)x, (float)y + 316, (float)pw, 200};
    char pd[128];
    char pb[128];

    /* Card 1: Algorithm Comparison */
    ui_draw_card(c_cmp, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Dijkstra vs Bellman-Ford Live Benchmark", x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});

    routing_format_path(&sim->cmp_dij, pd, sizeof(pd));
    routing_format_path(&sim->cmp_bf, pb, sizeof(pb));

    ui_draw_text_mono_sm(TextFormat("Path Cost:       Dijkstra: %.3f   |   Bellman-Ford: %.3f",
                                    sim->cmp_dij.cost, sim->cmp_bf.cost),
                         x + 14, y + 36, 12.0f, (Color){203, 213, 225, 255});

    ui_draw_text_mono_sm(TextFormat("Alg. Work:       Settled: %d nodes  |   Iterations: %d",
                                    sim->cmp_dij.settled, sim->cmp_bf.iterations),
                         x + 14, y + 56, 12.0f, (Color){203, 213, 225, 255});

    ui_draw_text_mono_sm(TextFormat("Exec. Time:      %ld us           |   %ld us",
                                    sim->cmp_dij.runtime_us, sim->cmp_bf.runtime_us),
                         x + 14, y + 76, 12.0f, (Color){52, 211, 153, 255});

    ui_draw_text_mono_sm(TextFormat("SPF Route (D):   %s", pd), x + 14, y + 100, 12.0f, (Color){251, 191, 36, 255});
    ui_draw_text_mono_sm(TextFormat("B-F Route (B):   %s", pb), x + 14, y + 122, 12.0f, (Color){56, 189, 248, 255});

    /* Card 2: Adjacency List */
    ui_draw_card(c_adj, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Topology Adjacency List", x + 12, y + 168, 13.0f, (Color){226, 232, 240, 255});
    {
        char line[128];
        int u;
        int row = 0;
        for (u = 0; u < NETSIM_MAX_NODES && row < 7; u++) {
            const AdjListNode *e;
            int n = 0;
            if (!sim->graph.node_used[u]) {
                continue;
            }
            n = snprintf(line, sizeof(line), "Node %d ->", u);
            for (e = sim->graph.adj[u]; e && n < 90; e = e->next) {
                n += snprintf(line + n, sizeof(line) - (size_t)n, " [%d: w=%.1f]", e->neighbor,
                              sim->graph.links[e->link_id].weight);
            }
            ui_draw_text_mono_sm(line, x + 14, y + 192 + row * 16, 11.0f, (Color){186, 200, 222, 255});
            row += 1;
        }
        if (row == 0) {
            ui_draw_text_sm("No nodes configured", x + 14, y + 196, 11.0f, (Color){148, 163, 184, 255});
        }
    }

    /* Card 3: Adjacency Matrix */
    ui_draw_card(c_mat, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Cost Matrix (Weights)", x + 12, y + 326, 13.0f, (Color){226, 232, 240, 255});
    {
        int ids[16];
        int n = 0;
        int i, j;
        for (i = 0; i < NETSIM_MAX_NODES && n < 8; i++) {
            if (sim->graph.node_used[i]) {
                ids[n++] = i;
            }
        }
        /* Col headers */
        for (j = 0; j < n; j++) {
            char h[8];
            snprintf(h, sizeof(h), "%d", ids[j]);
            ui_draw_text_mono_sm(h, x + 36 + j * 38, y + 348, 11.0f, (Color){96, 165, 250, 255});
        }
        /* Rows */
        for (i = 0; i < n; i++) {
            char h[8];
            snprintf(h, sizeof(h), "%d", ids[i]);
            ui_draw_text_mono_sm(h, x + 14, y + 366 + i * 18, 11.0f, (Color){96, 165, 250, 255});
            for (j = 0; j < n; j++) {
                double w_val = sim->graph.matrix[ids[i]][ids[j]];
                char cell[12];
                if (!netsim_finite(w_val)) {
                    snprintf(cell, sizeof(cell), "inf");
                    ui_draw_text_mono_sm(cell, x + 36 + j * 38, y + 366 + i * 18, 11.0f, (Color){100, 116, 139, 255});
                } else {
                    snprintf(cell, sizeof(cell), "%.1f", w_val);
                    ui_draw_text_mono_sm(cell, x + 36 + j * 38, y + 366 + i * 18, 11.0f,
                                         (ids[i] == ids[j]) ? (Color){148, 163, 184, 255} : (Color){241, 245, 249, 255});
                }
            }
        }
    }
}

static void draw_tab_logs(Simulator *sim, int x, int y, int pw)
{
    Rectangle c_log = {(float)x, (float)y, (float)pw, 440};
    int i;

    ui_draw_card(c_log, (Color){18, 23, 34, 255}, (Color){36, 46, 68, 255});
    ui_draw_text_bold("Real-Time Event History", x + 12, y + 10, 13.0f, (Color){226, 232, 240, 255});

    if (sim->ui.log_n == 0) {
        ui_draw_text_mono_sm("No events logged yet. Start simulation or create events.",
                             x + 14, y + 40, 11.0f, (Color){148, 163, 184, 255});
        return;
    }

    for (i = 0; i < sim->ui.log_n && i < 16; i++) {
        const char *line = sim->ui.log[i];
        Color col = (Color){203, 213, 225, 255};

        if (strstr(line, "fail") || strstr(line, "DOWN") || strstr(line, "drop")) {
            col = (Color){248, 113, 113, 255};
        } else if (strstr(line, "restored") || strstr(line, "UP") || strstr(line, "added")) {
            col = (Color){52, 211, 153, 255};
        } else if (strstr(line, "reroute") || strstr(line, "loaded")) {
            col = (Color){251, 191, 36, 255};
        }

        ui_draw_text_mono_sm(line, x + 14, y + 36 + i * 24, 11.5f, col);
        DrawLine(x + 14, y + 54 + i * 24, x + pw - 14, y + 54 + i * 24, (Color){28, 36, 52, 255});
    }
}

static void draw_status_bar(Simulator *sim, int w, int h)
{
    double avg, mn, mx;
    char path_str[64];
    Rectangle r_bar = {0, (float)(h - 28), (float)w, 28};
    float cur_x = 10.0f;

    DrawRectangleRec(r_bar, (Color){10, 13, 20, 255});
    DrawLine(0, h - 28, w, h - 28, (Color){30, 38, 56, 255});

    telemetry_latency_ms(&sim->tel, &avg, &mn, &mx);

    /* Time Badge */
    {
        char t_buf[32];
        snprintf(t_buf, sizeof(t_buf), "TIME %.3fs", sim->now);
        ui_draw_pill((Rectangle){cur_x, (float)h - 24, 85, 20}, t_buf, (Color){22, 32, 50, 255}, (Color){56, 189, 248, 255}, 1);
        cur_x += 92.0f;
    }

    /* Algo Badge */
    {
        const char *aname = sim->algo == ROUTING_BELLMAN_FORD ? "Bellman-Ford" : "Dijkstra";
        ui_draw_pill((Rectangle){cur_x, (float)h - 24, 90, 20}, aname, (Color){30, 42, 68, 255}, (Color){96, 165, 250, 255}, 1);
        cur_x += 98.0f;
    }

    /* PDR Badge */
    {
        char pdr_buf[32];
        snprintf(pdr_buf, sizeof(pdr_buf), "PDR %.1f%%", telemetry_pdr(&sim->tel));
        ui_draw_pill((Rectangle){cur_x, (float)h - 24, 80, 20}, pdr_buf, (Color){16, 50, 38, 255}, (Color){52, 211, 153, 255}, 1);
        cur_x += 88.0f;
    }

    /* Telemetry Counters */
    {
        char m_buf[128];
        snprintf(m_buf, sizeof(m_buf), "Flows:%d  In-flight:%d  Sent:%d  Recv:%d  Drops:%d  RTT:%.1fms",
                 sim->flow_count, sim->pool.live, sim->tel.sent, sim->tel.recv, sim->tel.dropped, avg);
        ui_draw_text_mono_sm(m_buf, cur_x, (float)h - 20, 11.0f, (Color){203, 213, 225, 255});
        cur_x += ui_measure_text_mono_sm(m_buf, 11.0f).x + 16.0f;
    }

    /* Path representation */
    if (sim->route.path_len > 0) {
        routing_format_path(&sim->route, path_str, sizeof(path_str));
        ui_draw_pill((Rectangle){cur_x, (float)h - 24, 150, 20}, path_str, (Color){45, 35, 20, 255}, (Color){251, 191, 36, 255}, 1);
    }

    /* Status string on the right */
    if (sim->status[0]) {
        ui_draw_text_right(w - 14, (float)h - 20, sim->status, 12.0f, (Color){148, 163, 184, 255});
    }
}

void ui_panels_draw(Simulator *sim)
{
    int w = sim->window_w;
    int h = sim->window_h;
    int right = 420;
    int x = w - right + 12;
    int y = 54;
    int pw = right - 24;

    /* 1. Top Toolbar */
    draw_top_bar(sim, w);

    /* 2. Right Inspector Sidebar */
    DrawRectangle(w - right, 54, right, h - 54 - 28, (Color){14, 18, 28, 255});
    DrawLine(w - right, 54, w - right, h - 28, (Color){36, 45, 66, 255});

    /* Tab switcher bar */
    {
        float tbw = (float)pw / 4.0f;
        if (ui_tab_button((Rectangle){(float)x, (float)y + 8, tbw - 2.0f, 30}, "Controls", sim->ui.active_tab == 0)) {
            sim->ui.active_tab = 0;
        }
        if (ui_tab_button((Rectangle){(float)x + tbw, (float)y + 8, tbw - 2.0f, 30}, "Elements", sim->ui.active_tab == 1)) {
            sim->ui.active_tab = 1;
        }
        if (ui_tab_button((Rectangle){(float)x + tbw * 2.0f, (float)y + 8, tbw - 2.0f, 30}, "Routing", sim->ui.active_tab == 2)) {
            sim->ui.active_tab = 2;
        }
        if (ui_tab_button((Rectangle){(float)x + tbw * 3.0f, (float)y + 8, tbw - 2.0f, 30}, "Logs", sim->ui.active_tab == 3)) {
            sim->ui.active_tab = 3;
        }
    }

    /* Content of active tab */
    switch (sim->ui.active_tab) {
        case 0:
            draw_tab_controls(sim, x, y + 46, pw);
            break;
        case 1:
            draw_tab_elements(sim, x, y + 46, pw);
            break;
        case 2:
            draw_tab_routing(sim, x, y + 46, pw);
            break;
        case 3:
            draw_tab_logs(sim, x, y + 46, pw);
            break;
        default:
            draw_tab_controls(sim, x, y + 46, pw);
            break;
    }

    /* 3. Bottom Status Bar */
    draw_status_bar(sim, w, h);
}

void ui_panels_handle(Simulator *sim)
{
    (void)sim;
}
