#ifndef NETSIM_SCENARIOS_H
#define NETSIM_SCENARIOS_H

struct Simulator;

/* Load a named lab scenario (topology + traffic + optional failure). */
int scenario_load(struct Simulator *sim, const char *name);

/* Run until EV_END in headless mode and export CSV. */
int scenario_run_headless(struct Simulator *sim, const char *name, const char *csv);

/* Print the list of supported scenario names. */
void scenario_list(void);

#endif /* NETSIM_SCENARIOS_H */
