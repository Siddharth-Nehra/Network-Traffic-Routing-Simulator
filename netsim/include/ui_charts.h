#ifndef NETSIM_UI_CHARTS_H
#define NETSIM_UI_CHARTS_H

struct Simulator;

/* Draw latency/throughput/queue/cwnd charts and the PDR bars. */
void ui_charts_draw(struct Simulator *sim);

#endif /* NETSIM_UI_CHARTS_H */
