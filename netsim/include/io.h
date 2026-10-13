#ifndef NETSIM_IO_H
#define NETSIM_IO_H

struct Simulator;

/* Save nodes and undirected links to a text .topo file. */
int io_save_topo(const struct Simulator *sim, const char *path);

/* Replace the current graph with a .topo file. */
int io_load_topo(struct Simulator *sim, const char *path);

/* Write per-flow and global metrics to CSV. */
int io_export_csv(struct Simulator *sim, const char *path, const char *scenario);

#endif /* NETSIM_IO_H */
