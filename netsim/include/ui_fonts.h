#ifndef NETSIM_UI_FONTS_H
#define NETSIM_UI_FONTS_H

#include "raylib.h"

struct Simulator;

/* Font lifecycle */
void ui_fonts_init(void);
void ui_fonts_shutdown(void);

/* Font handles */
Font ui_font_regular(void);
Font ui_font_regular_sm(void);
Font ui_font_bold(void);
Font ui_font_bold_sm(void);
Font ui_font_title(void);
Font ui_font_mono(void);
Font ui_font_mono_sm(void);

/* Text rendering */
void ui_draw_text(const char *text, float x, float y, float size, Color color);
void ui_draw_text_sm(const char *text, float x, float y, float size, Color color);
void ui_draw_text_bold(const char *text, float x, float y, float size, Color color);
void ui_draw_text_bold_sm(const char *text, float x, float y, float size, Color color);
void ui_draw_text_title(const char *text, float x, float y, float size, Color color);
void ui_draw_text_mono(const char *text, float x, float y, float size, Color color);
void ui_draw_text_mono_sm(const char *text, float x, float y, float size, Color color);

/* Text measurement */
Vector2 ui_measure_text(const char *text, float size);
Vector2 ui_measure_text_sm(const char *text, float size);
Vector2 ui_measure_text_bold(const char *text, float size);
Vector2 ui_measure_text_mono(const char *text, float size);
Vector2 ui_measure_text_mono_sm(const char *text, float size);
Vector2 ui_measure_text_title(const char *text, float size);

/* Text alignment helpers */
void ui_draw_text_centered(Rectangle r, const char *text, float size, Color color);
void ui_draw_text_centered_bold(Rectangle r, const char *text, float size, Color color);
void ui_draw_text_centered_mono(Rectangle r, const char *text, float size, Color color);
void ui_draw_text_right(float right_x, float y, const char *text, float size, Color color);
void ui_draw_text_right_mono(float right_x, float y, const char *text, float size, Color color);

/* UI Component drawing */
void ui_draw_card(Rectangle r, Color bg, Color border);
int ui_button(Rectangle r, const char *label, Color fill, Color text_color, int active);
int ui_tab_button(Rectangle r, const char *label, int active);
void ui_draw_pill(Rectangle r, const char *label, Color bg, Color text_color, int mono);
void ui_draw_badge(float x, float y, const char *label, Color bg, Color text_color);
void ui_draw_metric_pill(Rectangle r, const char *label, const char *val, Color accent_color);
void ui_input(struct Simulator *sim, Rectangle r, char *buf, int buflen, int foc, const char *label, const char *hint);

#endif /* NETSIM_UI_FONTS_H */
