#ifndef NETSIM_UI_CANVAS_H
#define NETSIM_UI_CANVAS_H

#include "sim.h"

int ui_canvas_init(Simulator *sim);
void ui_canvas_shutdown(Simulator *sim);
int ui_canvas_loop(Simulator *sim);
void ui_canvas_draw(const Simulator *sim);
void ui_canvas_handle_input(Simulator *sim);
void ui_layout_rects(const Simulator *sim, int *cx, int *cy, int *cw, int *ch);

#endif /* NETSIM_UI_CANVAS_H */
