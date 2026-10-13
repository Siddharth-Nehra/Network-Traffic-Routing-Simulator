#include "ui_canvas.h"
#include "scenarios.h"
#include "ui_charts.h"
#include "ui_fonts.h"
#include "ui_panels.h"

#include "raylib.h"

#define TOP_H 54
#define RIGHT_W 420
#define BOT_H 210
#define STATUS_H 28
#define CANVAS_BG (Color){15, 18, 26, 255}
#define NODE_RADIUS 18.0f

void ui_layout_rects(const Simulator *sim, int *cx, int *cy, int *cw, int *ch)
{
    *cx = 0;
    *cy = TOP_H;
    *cw = sim->window_w - RIGHT_W;
    *ch = sim->window_h - TOP_H - BOT_H - STATUS_H;
}

static float dist2(float x0, float y0, float x1, float y1)
{
    float dx = x0 - x1;
    float dy = y0 - y1;
    return dx * dx + dy * dy;
}

static float dist_point_seg(float px, float py, float ax, float ay, float bx, float by)
{
    float abx = bx - ax;
    float handy_y = by - ay;
    float len2 = abx * abx + handy_y * handy_y;
    float t;
    float qx;
    float qy;
    if (len2 < 1e-6f) {
        return sqrtf(dist2(px, py, ax, ay));
    }
    t = ((px - ax) * abx + (py - ay) * handy_y) / len2;
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    qx = ax + t * abx;
    qy = ay + t * handy_y;
    return sqrtf(dist2(px, py, qx, qy));
}

static int hit_node(const Simulator *sim, float mx, float my)
{
    int i;
    int best = -1;
    float best_d = (NODE_RADIUS + 4.0f) * (NODE_RADIUS + 4.0f);
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        float d;
        if (!sim->graph.node_used[i]) {
            continue;
        }
        d = dist2(mx, my, sim->graph.nodes[i].x, sim->graph.nodes[i].y);
        if (d <= best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

static int hit_link(const Simulator *sim, float mx, float my)
{
    int i;
    int best = -1;
    float best_d = 12.0f;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        const Link *l;
        const Node *a;
        const Node *b;
        float d;
        if (!sim->graph.link_used[i] || sim->graph.links[i].src > sim->graph.links[i].dst) {
            continue;
        }
        l = &sim->graph.links[i];
        a = graph_node_const(&sim->graph, l->src);
        b = graph_node_const(&sim->graph, l->dst);
        if (!a || !b) {
            continue;
        }
        d = dist_point_seg(mx, my, a->x, a->y, b->x, b->y);
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

static int path_contains_link(const RoutingResult *r, int u, int v)
{
    int i;
    for (i = 0; i + 1 < r->path_len; i++) {
        if ((r->path[i] == u && r->path[i + 1] == v) ||
            (r->path[i] == v && r->path[i + 1] == u)) {
            return 1;
        }
    }
    return 0;
}

static Color heatmap_color(double util)
{
    if (util < 0.0) {
        util = 0.0;
    }
    if (util > 1.0) {
        util = 1.0;
    }
    if (util < 0.5) {
        return (Color){(unsigned char)(40.0 + util * 380.0),
                       (unsigned char)(180.0 + util * 60.0), 120, 255};
    }
    return (Color){245, (unsigned char)(210.0 - (util - 0.5) * 360.0), 45, 255};
}

static Color flow_color(int flow_id)
{
    static const Color cols[] = {
        {56, 189, 248, 255},  /* Sky blue */
        {52, 211, 153, 255},  /* Mint emerald */
        {251, 146, 60, 255},  /* Bright amber */
        {192, 132, 252, 255}, /* Soft purple */
        {244, 114, 182, 255}, /* Rose pink */
        {250, 204, 21, 255}   /* Warm yellow */
    };
    if (flow_id < 0) {
        return RAYWHITE;
    }
    return cols[flow_id % 6];
}

static void fill_link_fields(Simulator *sim, int link_id)
{
    const Link *l = graph_link_const(&sim->graph, link_id);
    if (!l) {
        return;
    }
    snprintf(sim->ui.in_weight, sizeof(sim->ui.in_weight), "%.3f", l->weight);
    snprintf(sim->ui.in_bw, sizeof(sim->ui.in_bw), "%.1f", l->bandwidth_kbps);
    snprintf(sim->ui.in_delay, sizeof(sim->ui.in_delay), "%.1f", l->prop_delay_ms);
    snprintf(sim->ui.in_loss, sizeof(sim->ui.in_loss), "%.3f", l->loss_rate);
    snprintf(sim->ui.in_k, sizeof(sim->ui.in_k), "%d", l->queue_cap);
}

static void draw_canvas_grid(int cx, int cy, int cw, int ch)
{
    int step = 32;
    int start_x = cx + (step - (cx % step));
    int start_y = cy + (step - (cy % step));
    int x, y;
    for (x = start_x; x < cx + cw; x += step) {
        for (y = start_y; y < cy + ch; y += step) {
            DrawCircle(x, y, 1.2f, (Color){34, 42, 60, 255});
        }
    }
}

static void draw_queue_bar(const Node *a, const Node *b, const Link *l)
{
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float len = sqrtf(dx * dx + dy * dy);
    float px, py, frac;
    if (len < 1.0f) {
        return;
    }
    px = -dy / len * 9.0f;
    py = dx / len * 9.0f;
    frac = (l->queue_cap > 0) ? (float)queue_len(&l->queue) / (float)l->queue_cap : 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    /* Background bar line */
    DrawLineEx((Vector2){a->x + px, a->y + py},
               (Vector2){a->x + px + dx * 0.45f, a->y + py + dy * 0.45f},
               3.0f, (Color){30, 40, 60, 160});

    /* Active queue fill line */
    if (frac > 0.01f) {
        DrawLineEx((Vector2){a->x + px, a->y + py},
                   (Vector2){a->x + px + dx * (0.45f * frac), a->y + py + dy * (0.45f * frac)},
                   3.5f, (Color){248, 113, 113, 230});
    }
}

static void draw_links(const Simulator *sim)
{
    int i;
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        const Link *l;
        const Node *a;
        const Node *b;
        Color col;
        float thick;
        int in_path;
        char label[48];
        float mx, my;
        Vector2 lsz;
        Rectangle pr;

        if (!sim->graph.link_used[i] || sim->graph.links[i].src > sim->graph.links[i].dst) {
            continue;
        }
        l = &sim->graph.links[i];
        a = graph_node_const(&sim->graph, l->src);
        b = graph_node_const(&sim->graph, l->dst);
        if (!a || !b) {
            continue;
        }

        col = heatmap_color(l->utilization > sim->graph.links[l->reverse_id >= 0 ? l->reverse_id : i].utilization
                                    ? l->utilization
                                    : (l->reverse_id >= 0 ? sim->graph.links[l->reverse_id].utilization : l->utilization));
        in_path = path_contains_link(&sim->route, l->src, l->dst);

        if (in_path) {
            col = (Color){245, 158, 11, 255}; /* Glowing amber for shortest path */
        }
        if (l->state != LINK_UP) {
            col = (Color){150, 45, 45, 255}; /* Failed link */
        }

        thick = (i == sim->selected_link) ? 5.5f : (in_path ? 4.5f : 3.0f);

        /* Outer halo for selection or hover */
        if (i == sim->selected_link) {
            DrawLineEx((Vector2){a->x, a->y}, (Vector2){b->x, b->y}, thick + 6.0f, (Color){59, 130, 246, 120});
        } else if (i == sim->hover_link) {
            DrawLineEx((Vector2){a->x, a->y}, (Vector2){b->x, b->y}, thick + 4.0f, (Color){100, 140, 200, 80});
        }

        DrawLineEx((Vector2){a->x, a->y}, (Vector2){b->x, b->y}, thick, col);
        draw_queue_bar(a, b, l);

        /* Link metric label inside a crisp rounded pill badge */
        snprintf(label, sizeof(label), "w:%.1f  q:%d/%d", l->weight, queue_len(&l->queue), l->queue_cap);
        mx = (a->x + b->x) * 0.5f;
        my = (a->y + b->y) * 0.5f;
        lsz = ui_measure_text_mono_sm(label, 11.0f);

        pr = (Rectangle){mx - lsz.x * 0.5f - 6.0f, my - lsz.y * 0.5f - 3.0f, lsz.x + 12.0f, lsz.y + 6.0f};
        DrawRectangleRounded(pr, 0.45f, 4, (Color){16, 21, 32, 235});
        DrawRectangleRoundedLines(pr, 0.45f, 4,
                                  (i == sim->selected_link) ? (Color){59, 130, 246, 255} : (Color){45, 56, 82, 255});
        ui_draw_text_mono_sm(label, pr.x + 6.0f, pr.y + 3.0f, 11.0f,
                             (i == sim->selected_link) ? (Color){96, 165, 250, 255} : (Color){220, 230, 245, 255});
    }
}

static void draw_packets(const Simulator *sim)
{
    int i;
    for (i = 0; i < NETSIM_MAX_PACKETS; i++) {
        const Packet *p = packet_at_const(&sim->pool, i);
        const Link *l;
        const Node *a;
        const Node *b;
        float t = 0.0f;
        float x, y;
        Color c;

        if (!p || p->link_id < 0) {
            continue;
        }
        l = graph_link_const(&sim->graph, p->link_id);
        if (!l) {
            continue;
        }
        a = graph_node_const(&sim->graph, l->src);
        b = graph_node_const(&sim->graph, l->dst);
        if (!a || !b) {
            continue;
        }
        if (p->arrive_at > p->tx_start) {
            t = (float)((sim->now - p->tx_start) / (p->arrive_at - p->tx_start));
        } else if (l->busy_until > p->tx_start) {
            t = (float)((sim->now - p->tx_start) / (l->busy_until - p->tx_start));
        }
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        if (l->tx_pkt != i) {
            t = 0.12f;
        }
        x = a->x + (b->x - a->x) * t;
        y = a->y + (b->y - a->y) * t;

        c = flow_color(p->flow_id);
        if (p->is_ack) {
            /* Small ACK packet */
            DrawCircle((int)x, (int)y, 5.0f, ColorAlpha(c, 0.4f));
            DrawCircle((int)x, (int)y, 3.5f, RAYWHITE);
        } else {
            /* Data packet with glowing particle core */
            DrawCircle((int)x, (int)y, 8.0f, ColorAlpha(c, 0.35f));
            DrawCircle((int)x, (int)y, 5.5f, c);
            DrawCircle((int)x, (int)y, 2.5f, RAYWHITE);
        }
    }
}

static void draw_nodes(const Simulator *sim)
{
    int i;
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        const Node *n;
        Color fill = (Color){37, 99, 235, 255}; /* Vibrant Blue */
        Color ring = (Color){96, 165, 250, 255};
        char label[16];
        Rectangle nr;

        if (!sim->graph.node_used[i]) {
            continue;
        }
        n = &sim->graph.nodes[i];

        if (n->state != NODE_UP) {
            fill = (Color){75, 85, 99, 255}; /* Slate gray */
            ring = (Color){107, 114, 128, 255};
        } else if (i == sim->path_src) {
            fill = (Color){16, 185, 129, 255}; /* Emerald Green */
            ring = (Color){52, 211, 153, 255};
        } else if (i == sim->path_dst) {
            fill = (Color){249, 115, 22, 255}; /* Amber Orange */
            ring = (Color){251, 146, 60, 255};
        }

        /* Outer selection / hover halo */
        if (i == sim->selected_node) {
            DrawCircle((int)n->x, (int)n->y, NODE_RADIUS + 7.0f, (Color){255, 214, 10, 90});
            DrawCircleLines((int)n->x, (int)n->y, NODE_RADIUS + 5.0f, (Color){255, 214, 10, 255});
        } else if (i == sim->hover_node) {
            DrawCircle((int)n->x, (int)n->y, NODE_RADIUS + 5.0f, (Color){96, 165, 250, 70});
        }

        /* Outer border ring */
        DrawCircle((int)n->x, (int)n->y, NODE_RADIUS + 2.0f, ring);

        /* Main node body */
        DrawCircle((int)n->x, (int)n->y, NODE_RADIUS, fill);

        /* Top glossy reflection curve */
        DrawCircleGradient((Vector2){n->x, n->y - 4.0f}, NODE_RADIUS * 0.7f,
                           (Color){255, 255, 255, 60}, (Color){255, 255, 255, 0});

        /* Centered node ID in bold */
        snprintf(label, sizeof(label), "%d", n->id);
        nr = (Rectangle){n->x - NODE_RADIUS, n->y - NODE_RADIUS, NODE_RADIUS * 2.0f, NODE_RADIUS * 2.0f};
        ui_draw_text_centered_bold(nr, label, 15.0f, RAYWHITE);

        /* Role badge beneath node */
        if (i == sim->path_src) {
            ui_draw_badge(n->x - 14.0f, n->y + NODE_RADIUS + 5.0f, "SRC",
                          (Color){6, 78, 59, 240}, (Color){52, 211, 153, 255});
        } else if (i == sim->path_dst) {
            ui_draw_badge(n->x - 14.0f, n->y + NODE_RADIUS + 5.0f, "DST",
                          (Color){124, 45, 18, 240}, (Color){251, 146, 60, 255});
        }
    }
}

static void draw_flashes(const Simulator *sim)
{
    int i;
    for (i = 0; i < NETSIM_MAX_FLASH; i++) {
        if (sim->flashes[i].used && sim->flashes[i].until > sim->now) {
            float rem = (float)(sim->flashes[i].until - sim->now);
            float progress = rem / 0.5f;
            if (progress > 1.0f) progress = 1.0f;
            if (progress < 0.0f) progress = 0.0f;

            float r = 16.0f + (1.0f - progress) * 22.0f;
            DrawCircleLines((int)sim->flashes[i].x, (int)sim->flashes[i].y, r,
                            (Color){239, 68, 68, (unsigned char)(progress * 220)});
            DrawCircle((int)sim->flashes[i].x, (int)sim->flashes[i].y, 14.0f,
                       (Color){239, 68, 68, (unsigned char)(progress * 80)});
            ui_draw_badge(sim->flashes[i].x - 18.0f, sim->flashes[i].y - 24.0f - (1.0f - progress) * 8.0f,
                          "DROP", (Color){153, 27, 27, 240}, (Color){254, 202, 202, 255});
        }
    }
}

int ui_canvas_init(Simulator *sim)
{
    if (!sim) {
        return -1;
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(sim->window_w, sim->window_h, "Network Traffic & Routing Simulator");
    if (!IsWindowReady()) {
        snprintf(sim->last_error, sizeof(sim->last_error), "failed to open window");
        return -1;
    }
    SetTargetFPS(60);

    /* Initialize high-legibility TrueType typography engine */
    ui_fonts_init();

    return 0;
}

void ui_canvas_shutdown(Simulator *sim)
{
    (void)sim;
    ui_fonts_shutdown();
    if (IsWindowReady()) {
        CloseWindow();
    }
}

static int mouse_in_canvas(const Simulator *sim, Vector2 mouse)
{
    int cx, cy, cw, ch;
    ui_layout_rects(sim, &cx, &cy, &cw, &ch);
    return mouse.x >= (float)cx && mouse.x <= (float)(cx + cw) && mouse.y >= (float)cy &&
           mouse.y <= (float)(cy + ch);
}

static void handle_new_node(Simulator *sim, float mx, float my)
{
    int id = graph_add_node(&sim->graph, mx, my);
    if (id < 0) {
        return;
    }
    if (sim->path_src < 0) {
        sim->path_src = id;
    } else if (sim->path_dst < 0 && id != sim->path_src) {
        sim->path_dst = id;
    }
    sim->selected_node = id;
    sim->selected_link = -1;
    sim_recompute_route(sim);
}

static void handle_node_click(Simulator *sim, int id)
{
    if (sim->selected_node >= 0 && sim->selected_node != id) {
        graph_add_link(&sim->graph, sim->selected_node, id, NETSIM_DEFAULT_WEIGHT,
                       NETSIM_DEFAULT_BW_KBPS, NETSIM_DEFAULT_DELAY_MS, NETSIM_DEFAULT_LOSS,
                       20);
        sim->selected_node = id;
        sim_recompute_route(sim);
        return;
    }
    sim->selected_node = id;
    sim->selected_link = -1;
}

static void handle_delete(Simulator *sim)
{
    if (sim->selected_node >= 0) {
        int gone = sim->selected_node;
        graph_remove_node(&sim->graph, gone);
        if (sim->path_src == gone) {
            sim->path_src = -1;
        }
        if (sim->path_dst == gone) {
            sim->path_dst = -1;
        }
        sim->selected_node = -1;
        sim_recompute_route(sim);
        return;
    }
    if (sim->selected_link >= 0) {
        graph_remove_link(&sim->graph, sim->selected_link);
        sim->selected_link = -1;
        sim_recompute_route(sim);
    }
}

void ui_canvas_handle_input(Simulator *sim)
{
    Vector2 mouse = GetMousePosition();
    int over_node;
    int over_link;
    sim->window_w = GetScreenWidth();
    sim->window_h = GetScreenHeight();

    if (IsKeyPressed(KEY_F12)) {
        char path[160];
        snprintf(path, sizeof(path), "docs/screenshots/shot_%d.png", sim->screenshot_n);
        TakeScreenshot(path);
        sim_log(sim, "saved %s", path);
        sim->screenshot_n += 1;
    }
    if (IsKeyPressed(KEY_SPACE) && sim->ui.focus == UI_FOC_NONE) {
        if (sim->playing) {
            sim_pause(sim);
        } else {
            sim_play(sim);
        }
    }
    if (IsKeyPressed(KEY_F) && sim->ui.focus == UI_FOC_NONE) {
        sim_fail_selected(sim);
    }

    if (!mouse_in_canvas(sim, mouse) || sim->ui.focus != UI_FOC_NONE) {
        return;
    }

    over_node = hit_node(sim, mouse.x, mouse.y);
    over_link = (over_node >= 0) ? -1 : hit_link(sim, mouse.x, mouse.y);
    sim->hover_node = over_node;
    sim->hover_link = over_link;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (over_node >= 0) {
            sim->dragging = 1;
            sim->drag_node = over_node;
            sim->drag_off_x = mouse.x - sim->graph.nodes[over_node].x;
            sim->drag_off_y = mouse.y - sim->graph.nodes[over_node].y;
            handle_node_click(sim, over_node);
        } else if (over_link >= 0) {
            sim->selected_link = over_link;
            sim->selected_node = -1;
            fill_link_fields(sim, over_link);
            sim->ui.active_tab = 1; /* Switch to element editor tab automatically */
        } else {
            handle_new_node(sim, mouse.x, mouse.y);
        }
    }

    if (sim->dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && sim->drag_node >= 0) {
        sim->graph.nodes[sim->drag_node].x = mouse.x - sim->drag_off_x;
        sim->graph.nodes[sim->drag_node].y = mouse.y - sim->drag_off_y;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        sim->dragging = 0;
        sim->drag_node = -1;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && over_node >= 0) {
        if (sim->path_src < 0 || over_node == sim->path_dst) {
            sim->path_src = over_node;
        } else {
            sim->path_dst = over_node;
        }
        sim_recompute_route(sim);
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        handle_delete(sim);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        sim->selected_node = -1;
        sim->selected_link = -1;
        sim->ui.focus = UI_FOC_NONE;
    }
}

void ui_canvas_draw(const Simulator *sim)
{
    int cx, cy, cw, ch;
    ui_layout_rects(sim, &cx, &cy, &cw, &ch);

    BeginDrawing();
    ClearBackground(CANVAS_BG);

    /* Canvas area with blueprint dot grid */
    BeginScissorMode(cx, cy, cw, ch);
    draw_canvas_grid(cx, cy, cw, ch);
    draw_links(sim);
    draw_packets(sim);
    draw_flashes(sim);
    draw_nodes(sim);

    /* Quick canvas hint on bottom-left */
    {
        char hint[128];
        snprintf(hint, sizeof(hint), "Canvas: Left click to add node/link | Drag to move | Right click for SRC/DST path");
        ui_draw_text_sm(hint, (float)cx + 14.0f, (float)(cy + ch - 22), 12.0f, (Color){110, 125, 150, 180});
    }

    EndScissorMode();

    /* Render telemetry charts, panels, toolbar, and status bar */
    ui_charts_draw((Simulator *)sim);
    ui_panels_draw((Simulator *)sim);

    EndDrawing();
}

int ui_canvas_loop(Simulator *sim)
{
    if (ui_canvas_init(sim) != 0) {
        return -1;
    }
    if (sim->graph.node_count == 0) {
        scenario_load(sim, "normal_traffic");
    }
    while (!WindowShouldClose()) {
        ui_canvas_handle_input(sim);
        if (sim->playing) {
            sim_advance(sim, (double)GetFrameTime() * sim->speed);
        }
        ui_canvas_draw(sim);
    }
    ui_canvas_shutdown(sim);
    return 0;
}
