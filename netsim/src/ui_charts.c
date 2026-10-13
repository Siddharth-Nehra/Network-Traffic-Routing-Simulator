#include "ui_charts.h"
#include "sim.h"
#include "ui_fonts.h"

#include "raylib.h"
#include <stdio.h>

static Color chart_color(int k)
{
    static const Color cols[] = {
        {56, 189, 248, 255},  /* Cyan / Blue */
        {52, 211, 153, 255},  /* Emerald */
        {251, 146, 60, 255},  /* Amber */
        {192, 132, 252, 255}, /* Violet */
        {244, 114, 182, 255}  /* Rose */
    };
    return cols[k % 5];
}

/* Map a ring-buffer index in chronological order. */
static int chart_at(const Telemetry *t, int i)
{
    if (t->chart_n < NETSIM_CHART) {
        return i;
    }
    return (t->chart_head + i) % NETSIM_CHART;
}

static void draw_series(Rectangle r, const Telemetry *t, const double *series, const char *title,
                        const char *unit, Color col)
{
    int n = t->chart_n;
    int i;
    double mx = 0.001;
    double last_val = 0.0;
    char val_str[32];
    char mx_str[32];
    float plot_x, plot_y, plot_w, plot_h;

    /* Card background & border */
    ui_draw_card(r, (Color){16, 21, 32, 255}, (Color){38, 48, 70, 255});

    /* Header indicator dot */
    DrawCircle((int)(r.x + 12), (int)(r.y + 16), 4.0f, col);
    ui_draw_text_bold(title, r.x + 22.0f, r.y + 9.0f, 13.0f, RAYWHITE);

    if (n > 0) {
        last_val = series[chart_at(t, n - 1)];
        for (i = 0; i < n; i++) {
            double v = series[chart_at(t, i)];
            if (v > mx) {
                mx = v;
            }
        }
    }

    /* Live current value badge on top right */
    if (last_val >= 100.0) {
        snprintf(val_str, sizeof(val_str), "%.1f %s", last_val, unit);
    } else {
        snprintf(val_str, sizeof(val_str), "%.2f %s", last_val, unit);
    }
    ui_draw_text_right_mono(r.x + r.width - 10.0f, r.y + 10.0f, val_str, 12.0f, col);

    /* Plot area bounds */
    plot_x = r.x + 8.0f;
    plot_y = r.y + 32.0f;
    plot_w = r.width - 16.0f;
    plot_h = r.height - 40.0f;

    /* Subtle horizontal grid lines */
    DrawLine((int)plot_x, (int)(plot_y + plot_h * 0.25f), (int)(plot_x + plot_w), (int)(plot_y + plot_h * 0.25f), (Color){26, 33, 48, 255});
    DrawLine((int)plot_x, (int)(plot_y + plot_h * 0.50f), (int)(plot_x + plot_w), (int)(plot_y + plot_h * 0.50f), (Color){26, 33, 48, 255});
    DrawLine((int)plot_x, (int)(plot_y + plot_h * 0.75f), (int)(plot_x + plot_w), (int)(plot_y + plot_h * 0.75f), (Color){26, 33, 48, 255});
    DrawLine((int)plot_x, (int)(plot_y + plot_h), (int)(plot_x + plot_w), (int)(plot_y + plot_h), (Color){34, 43, 62, 255});

    /* Max scale watermark label */
    snprintf(mx_str, sizeof(mx_str), "max %.1f", mx);
    ui_draw_text_mono_sm(mx_str, plot_x + 4.0f, plot_y + 2.0f, 10.0f, (Color){100, 115, 140, 200});

    if (n < 2) {
        ui_draw_text_centered_mono((Rectangle){plot_x, plot_y, plot_w, plot_h}, "collecting data...", 11.0f, (Color){100, 115, 135, 255});
        return;
    }

    /* Shaded area under the curve & line stroke */
    for (i = 1; i < n; i++) {
        float x0 = plot_x + (float)(i - 1) * plot_w / (float)(n - 1);
        float x1 = plot_x + (float)i * plot_w / (float)(n - 1);
        float y0 = plot_y + plot_h - (float)(series[chart_at(t, i - 1)] / mx) * (plot_h - 4.0f);
        float y1 = plot_y + plot_h - (float)(series[chart_at(t, i)] / mx) * (plot_h - 4.0f);

        /* Soft shaded area fill under line */
        DrawTriangle((Vector2){x0, plot_y + plot_h}, (Vector2){x1, plot_y + plot_h}, (Vector2){x1, y1}, ColorAlpha(col, 0.12f));
        DrawTriangle((Vector2){x0, plot_y + plot_h}, (Vector2){x1, y1}, (Vector2){x0, y0}, ColorAlpha(col, 0.12f));

        /* Line stroke */
        DrawLineEx((Vector2){x0, y0}, (Vector2){x1, y1}, 2.0f, col);
    }
}

static void draw_pdr_bars(Simulator *sim, Rectangle r)
{
    int i;
    int n = 0;
    int idx[NETSIM_MAX_FLOWS];
    float plot_x, plot_y, plot_w, plot_h;
    double avg_pdr;
    char avg_str[32];

    ui_draw_card(r, (Color){16, 21, 32, 255}, (Color){38, 48, 70, 255});

    DrawCircle((int)(r.x + 12), (int)(r.y + 16), 4.0f, chart_color(4));
    ui_draw_text_bold("Flow PDR", r.x + 22.0f, r.y + 9.0f, 13.0f, RAYWHITE);

    avg_pdr = telemetry_pdr(&sim->tel);
    snprintf(avg_str, sizeof(avg_str), "avg %.1f%%", avg_pdr);
    ui_draw_text_right_mono(r.x + r.width - 10.0f, r.y + 10.0f, avg_str, 12.0f, (Color){52, 211, 153, 255});

    plot_x = r.x + 8.0f;
    plot_y = r.y + 32.0f;
    plot_w = r.width - 16.0f;
    plot_h = r.height - 40.0f;

    /* 100% reference line */
    DrawLine((int)plot_x, (int)(plot_y + 4.0f), (int)(plot_x + plot_w), (int)(plot_y + 4.0f), (Color){52, 211, 153, 90});
    ui_draw_text_mono_sm("100%", plot_x + 4.0f, plot_y + 2.0f, 10.0f, (Color){52, 211, 153, 140});

    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        if (sim->flows[i].used) {
            idx[n++] = i;
        }
    }

    if (n == 0) {
        ui_draw_text_centered_mono((Rectangle){plot_x, plot_y, plot_w, plot_h}, "no active flows", 11.0f, (Color){100, 115, 135, 255});
        return;
    }

    for (i = 0; i < n && i < 8; i++) {
        Flow *f = &sim->flows[idx[i]];
        double pdr = f->sent ? 100.0 * (double)f->recv / (double)f->sent : 0.0;
        float slot_w = plot_w / (float)n;
        float bar_w = slot_w > 28.0f ? 24.0f : (slot_w - 6.0f);
        if (bar_w < 6.0f) bar_w = 6.0f;

        float max_h = plot_h - 22.0f;
        float h = (float)(pdr / 100.0) * max_h;
        if (h < 2.0f && pdr > 0.0) h = 2.0f;

        float bx = plot_x + (float)i * slot_w + (slot_w - bar_w) * 0.5f;
        float by = plot_y + max_h - h;
        Color c = chart_color(i);
        char pdr_txt[16];
        char fl_txt[16];

        /* Bar rectangle */
        DrawRectangleRounded((Rectangle){bx, by, bar_w, h + 2.0f}, 0.25f, 4, c);

        /* Percentage text on top of bar */
        snprintf(pdr_txt, sizeof(pdr_txt), "%.0f%%", pdr);
        ui_draw_text_centered_mono((Rectangle){plot_x + (float)i * slot_w, by - 14.0f, slot_w, 12.0f},
                                  pdr_txt, 10.0f, RAYWHITE);

        /* Flow ID label below bar */
        snprintf(fl_txt, sizeof(fl_txt), "F%d", idx[i]);
        ui_draw_text_centered_bold((Rectangle){plot_x + (float)i * slot_w, plot_y + max_h + 4.0f, slot_w, 14.0f},
                                  fl_txt, 11.0f, (Color){170, 185, 210, 255});
    }
}

void ui_charts_draw(Simulator *sim)
{
    int w = sim->window_w;
    int h = sim->window_h;
    int right_panel_w = 420;
    int total_w = w - right_panel_w;
    int bot = 210;
    int status = 28;
    int y = h - status - bot;
    float pad = 6.0f;
    float cw = ((float)total_w - pad * 6.0f) / 5.0f;

    Rectangle a = {pad, (float)y + pad, cw, (float)bot - pad * 2.0f};
    Rectangle b = {pad * 2.0f + cw, (float)y + pad, cw, (float)bot - pad * 2.0f};
    Rectangle c = {pad * 3.0f + 2.0f * cw, (float)y + pad, cw, (float)bot - pad * 2.0f};
    Rectangle d = {pad * 4.0f + 3.0f * cw, (float)y + pad, cw, (float)bot - pad * 2.0f};
    Rectangle e = {pad * 5.0f + 4.0f * cw, (float)y + pad, cw, (float)bot - pad * 2.0f};

    /* Background panel container */
    DrawRectangle(0, y, total_w, bot, (Color){12, 15, 22, 255});
    DrawLine(0, y, total_w, y, (Color){34, 43, 62, 255});

    draw_series(a, &sim->tel, sim->tel.lat_hist, "Latency", "ms", chart_color(0));
    draw_series(b, &sim->tel, sim->tel.thr_hist, "Throughput", "kbps", chart_color(1));
    draw_series(c, &sim->tel, sim->tel.qlen_hist, "Queue Depth", "pkts", chart_color(2));
    draw_series(d, &sim->tel, sim->tel.cwnd_hist, "TCP Cwnd", "pkts", chart_color(3));
    draw_pdr_bars(sim, e);
}
