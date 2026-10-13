#ifndef NETSIM_UI_PANELS_H
#define NETSIM_UI_PANELS_H

struct Simulator;

/* Draw the top toolbar, right inspector, and status bar. */
void ui_panels_draw(struct Simulator *sim);

/* Handle clicks and typing for panels and toolbar. */
void ui_panels_handle(struct Simulator *sim);

#endif /* NETSIM_UI_PANELS_H */
