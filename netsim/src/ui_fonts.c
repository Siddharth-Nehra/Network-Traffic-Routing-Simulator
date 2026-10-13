#include "ui_fonts.h"
#include "sim.h"

#include <stdio.h>
#include <string.h>

static Font s_font_reg = {0};
static Font s_font_reg_sm = {0};
static Font s_font_bold = {0};
static Font s_font_bold_sm = {0};
static Font s_font_title = {0};
static Font s_font_mono = {0};
static Font s_font_mono_sm = {0};
static int s_fonts_loaded = 0;

static const char *find_font_path(const char *candidates[])
{
    int i;
    for (i = 0; candidates[i] != NULL; i++) {
        if (FileExists(candidates[i])) {
            return candidates[i];
        }
    }
    return NULL;
}

static Font safe_load_font(const char *path, int size)
{
    Font f = {0};
    if (path && FileExists(path)) {
        f = LoadFontEx(path, size, NULL, 0);
        if (f.baseSize > 0) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
            return f;
        }
    }
    return GetFontDefault();
}

void ui_fonts_init(void)
{
    const char *reg_candidates[] = {
        "assets/fonts/Arial.ttf",
        "netsim/assets/fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
        NULL
    };
    const char *bold_candidates[] = {
        "assets/fonts/Arial-Bold.ttf",
        "netsim/assets/fonts/Arial-Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        NULL
    };
    const char *mono_candidates[] = {
        "assets/fonts/SFNSMono.ttf",
        "netsim/assets/fonts/SFNSMono.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "C:/Windows/Fonts/consola.ttf",
        NULL
    };

    const char *reg_path = find_font_path(reg_candidates);
    const char *bold_path = find_font_path(bold_candidates);
    const char *mono_path = find_font_path(mono_candidates);

    if (!bold_path && reg_path) {
        bold_path = reg_path;
    }
    if (!mono_path && reg_path) {
        mono_path = reg_path;
    }

    s_font_reg = safe_load_font(reg_path, 15);
    s_font_reg_sm = safe_load_font(reg_path, 13);
    s_font_bold = safe_load_font(bold_path, 15);
    s_font_bold_sm = safe_load_font(bold_path, 13);
    s_font_title = safe_load_font(bold_path, 20);
    s_font_mono = safe_load_font(mono_path, 14);
    s_font_mono_sm = safe_load_font(mono_path, 12);

    s_fonts_loaded = 1;
}

void ui_fonts_shutdown(void)
{
    if (!s_fonts_loaded) {
        return;
    }
    if (s_font_reg.baseSize > 0 && s_font_reg.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_reg);
    }
    if (s_font_reg_sm.baseSize > 0 && s_font_reg_sm.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_reg_sm);
    }
    if (s_font_bold.baseSize > 0 && s_font_bold.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_bold);
    }
    if (s_font_bold_sm.baseSize > 0 && s_font_bold_sm.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_bold_sm);
    }
    if (s_font_title.baseSize > 0 && s_font_title.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_title);
    }
    if (s_font_mono.baseSize > 0 && s_font_mono.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_mono);
    }
    if (s_font_mono_sm.baseSize > 0 && s_font_mono_sm.texture.id != GetFontDefault().texture.id) {
        UnloadFont(s_font_mono_sm);
    }
    s_fonts_loaded = 0;
}

Font ui_font_regular(void) { return s_font_reg.baseSize > 0 ? s_font_reg : GetFontDefault(); }
Font ui_font_regular_sm(void) { return s_font_reg_sm.baseSize > 0 ? s_font_reg_sm : GetFontDefault(); }
Font ui_font_bold(void) { return s_font_bold.baseSize > 0 ? s_font_bold : GetFontDefault(); }
Font ui_font_bold_sm(void) { return s_font_bold_sm.baseSize > 0 ? s_font_bold_sm : GetFontDefault(); }
Font ui_font_title(void) { return s_font_title.baseSize > 0 ? s_font_title : GetFontDefault(); }
Font ui_font_mono(void) { return s_font_mono.baseSize > 0 ? s_font_mono : GetFontDefault(); }
Font ui_font_mono_sm(void) { return s_font_mono_sm.baseSize > 0 ? s_font_mono_sm : GetFontDefault(); }

void ui_draw_text(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_regular(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_sm(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_regular_sm(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_bold(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_bold(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_bold_sm(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_bold_sm(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_title(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_title(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_mono(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_mono(), text, (Vector2){x, y}, size, 0.0f, color);
}

void ui_draw_text_mono_sm(const char *text, float x, float y, float size, Color color)
{
    if (!text || !text[0]) return;
    DrawTextEx(ui_font_mono_sm(), text, (Vector2){x, y}, size, 0.0f, color);
}

Vector2 ui_measure_text(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_regular(), text, size, 0.0f);
}

Vector2 ui_measure_text_sm(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_regular_sm(), text, size, 0.0f);
}

Vector2 ui_measure_text_bold(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_bold(), text, size, 0.0f);
}

Vector2 ui_measure_text_mono(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_mono(), text, size, 0.0f);
}

Vector2 ui_measure_text_mono_sm(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_mono_sm(), text, size, 0.0f);
}

Vector2 ui_measure_text_title(const char *text, float size)
{
    if (!text || !text[0]) return (Vector2){0, 0};
    return MeasureTextEx(ui_font_title(), text, size, 0.0f);
}

void ui_draw_text_centered(Rectangle r, const char *text, float size, Color color)
{
    Vector2 sz = ui_measure_text(text, size);
    float x = r.x + (r.width - sz.x) * 0.5f;
    float y = r.y + (r.height - sz.y) * 0.5f;
    ui_draw_text(text, x, y, size, color);
}

void ui_draw_text_centered_bold(Rectangle r, const char *text, float size, Color color)
{
    Vector2 sz = ui_measure_text_bold(text, size);
    float x = r.x + (r.width - sz.x) * 0.5f;
    float y = r.y + (r.height - sz.y) * 0.5f;
    ui_draw_text_bold(text, x, y, size, color);
}

void ui_draw_text_centered_mono(Rectangle r, const char *text, float size, Color color)
{
    Vector2 sz = ui_measure_text_mono(text, size);
    float x = r.x + (r.width - sz.x) * 0.5f;
    float y = r.y + (r.height - sz.y) * 0.5f;
    ui_draw_text_mono(text, x, y, size, color);
}

void ui_draw_text_right(float right_x, float y, const char *text, float size, Color color)
{
    Vector2 sz = ui_measure_text(text, size);
    ui_draw_text(text, right_x - sz.x, y, size, color);
}

void ui_draw_text_right_mono(float right_x, float y, const char *text, float size, Color color)
{
    Vector2 sz = ui_measure_text_mono(text, size);
    ui_draw_text_mono(text, right_x - sz.x, y, size, color);
}

void ui_draw_card(Rectangle r, Color bg, Color border)
{
    DrawRectangleRounded(r, 0.08f, 4, bg);
    DrawRectangleRoundedLines(r, 0.08f, 4, border);
}

static int in_rect(Rectangle r, Vector2 p)
{
    return p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height;
}

int ui_button(Rectangle r, const char *label, Color fill, Color text_color, int active)
{
    Vector2 m = GetMousePosition();
    int hovered = in_rect(r, m);
    Color bg = fill;
    Color border = ColorAlpha(fill, 0.8f);

    if (active) {
        bg = ColorBrightness(fill, 0.25f);
        border = (Color){255, 214, 10, 255};
    } else if (hovered) {
        bg = ColorBrightness(fill, 0.15f);
        border = ColorBrightness(fill, 0.35f);
    }

    DrawRectangleRounded(r, 0.22f, 4, bg);
    DrawRectangleRoundedLines(r, 0.22f, 4, border);
    ui_draw_text_centered_bold(r, label, 13.0f, text_color);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return 1;
    }
    return 0;
}

int ui_tab_button(Rectangle r, const char *label, int active)
{
    Vector2 m = GetMousePosition();
    int hovered = in_rect(r, m);
    Color bg = active ? (Color){37, 99, 235, 255} : (hovered ? (Color){30, 38, 56, 255} : (Color){20, 25, 38, 255});
    Color border = active ? (Color){96, 165, 250, 255} : (Color){45, 55, 80, 255};
    Color text_col = active ? RAYWHITE : (hovered ? (Color){220, 230, 245, 255} : (Color){150, 165, 185, 255});

    DrawRectangleRounded(r, 0.25f, 4, bg);
    DrawRectangleRoundedLines(r, 0.25f, 4, border);
    ui_draw_text_centered_bold(r, label, 13.0f, text_col);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return 1;
    }
    return 0;
}

void ui_draw_pill(Rectangle r, const char *label, Color bg, Color text_color, int mono)
{
    DrawRectangleRounded(r, 0.5f, 4, bg);
    DrawRectangleRoundedLines(r, 0.5f, 4, ColorAlpha(text_color, 0.4f));
    if (mono) {
        ui_draw_text_centered_mono(r, label, 11.0f, text_color);
    } else {
        ui_draw_text_centered_bold(r, label, 11.0f, text_color);
    }
}

void ui_draw_badge(float x, float y, const char *label, Color bg, Color text_color)
{
    Vector2 sz = ui_measure_text_bold(label, 11.0f);
    Rectangle r = {x, y, sz.x + 12.0f, sz.y + 6.0f};
    DrawRectangleRounded(r, 0.4f, 4, bg);
    DrawRectangleRoundedLines(r, 0.4f, 4, ColorAlpha(text_color, 0.4f));
    ui_draw_text_bold(label, x + 6.0f, y + 3.0f, 11.0f, text_color);
}

void ui_draw_metric_pill(Rectangle r, const char *label, const char *val, Color accent_color)
{
    DrawRectangleRounded(r, 0.25f, 4, (Color){18, 23, 34, 255});
    DrawRectangleRoundedLines(r, 0.25f, 4, (Color){38, 48, 70, 255});

    float split = r.x + 8.0f;
    ui_draw_text_sm(label, split, r.y + (r.height - 12.0f) * 0.5f - 1.0f, 11.0f, (Color){140, 155, 175, 255});

    Vector2 vsz = ui_measure_text_mono(val, 12.0f);
    ui_draw_text_mono(val, r.x + r.width - vsz.x - 8.0f, r.y + (r.height - 12.0f) * 0.5f - 1.0f, 12.0f, accent_color);
}

void ui_input(Simulator *sim, Rectangle r, char *buf, int buflen, int foc, const char *label, const char *hint)
{
    Vector2 m = GetMousePosition();
    int active = (sim->ui.focus == foc);
    int hovered = in_rect(r, m);

    Color bg = active ? (Color){28, 38, 60, 255} : (hovered ? (Color){22, 28, 44, 255} : (Color){16, 20, 32, 255});
    Color border = active ? (Color){59, 130, 246, 255} : (hovered ? (Color){70, 85, 115, 255} : (Color){40, 50, 75, 255});

    if (label && label[0]) {
        ui_draw_text_sm(label, r.x, r.y - 14.0f, 11.0f, (Color){150, 165, 190, 255});
    }

    DrawRectangleRounded(r, 0.25f, 4, bg);
    DrawRectangleRoundedLines(r, 0.25f, 4, border);

    if (buf && buf[0]) {
        ui_draw_text_mono(buf, r.x + 8.0f, r.y + (r.height - 14.0f) * 0.5f, 13.0f, RAYWHITE);
        if (active) {
            Vector2 sz = ui_measure_text_mono(buf, 13.0f);
            if (((int)(GetTime() * 2.0)) % 2 == 0) {
                DrawLine((int)(r.x + 8.0f + sz.x + 2.0f), (int)(r.y + 4.0f),
                         (int)(r.x + 8.0f + sz.x + 2.0f), (int)(r.y + r.height - 4.0f), (Color){59, 130, 246, 255});
            }
        }
    } else if (hint && hint[0]) {
        ui_draw_text_sm(hint, r.x + 8.0f, r.y + (r.height - 12.0f) * 0.5f, 12.0f, (Color){100, 115, 135, 255});
        if (active && (((int)(GetTime() * 2.0)) % 2 == 0)) {
            DrawLine((int)(r.x + 8.0f), (int)(r.y + 4.0f),
                     (int)(r.x + 8.0f), (int)(r.y + r.height - 4.0f), (Color){59, 130, 246, 255});
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hovered) {
            sim->ui.focus = foc;
        } else if (active) {
            sim->ui.focus = UI_FOC_NONE;
        }
    }

    if (!active) {
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        sim->ui.focus = UI_FOC_NONE;
        if (foc >= UI_FOC_W && foc <= UI_FOC_K) {
            sim_apply_link_fields(sim);
        }
        return;
    }

    if (IsKeyPressed(KEY_BACKSPACE) && buf && buf[0]) {
        buf[strlen(buf) - 1] = '\0';
    }

    {
        int ch = GetCharPressed();
        while (ch > 0) {
            int n = (int)strlen(buf);
            if (n < buflen - 1 && ch >= 32 && ch < 127) {
                buf[n] = (char)ch;
                buf[n + 1] = '\0';
            }
            ch = GetCharPressed();
        }
    }
}
