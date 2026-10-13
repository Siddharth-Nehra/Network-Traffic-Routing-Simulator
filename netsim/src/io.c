#include "io.h"
#include "sim.h"

int io_save_topo(const Simulator *sim, const char *path)
{
    FILE *fp;
    int i;
    if (!sim || !path) {
        return -1;
    }
    fp = fopen(path, "w");
    if (!fp) {
        perror(path);
        return -1;
    }
    fprintf(fp, "NETSIM 1\n");
    fprintf(fp, "PATH %d %d\n", sim->path_src, sim->path_dst);
    for (i = 0; i < NETSIM_MAX_NODES; i++) {
        const Node *n;
        if (!sim->graph.node_used[i]) {
            continue;
        }
        n = &sim->graph.nodes[i];
        fprintf(fp, "NODE %d %s %.3f %.3f %s\n", n->id, n->name, n->x, n->y,
                n->state == NODE_UP ? "UP" : "DOWN");
    }
    for (i = 0; i < NETSIM_MAX_LINKS; i++) {
        const Link *l;
        if (!sim->graph.link_used[i] || sim->graph.links[i].src > sim->graph.links[i].dst) {
            continue;
        }
        l = &sim->graph.links[i];
        fprintf(fp, "LINK %d %d %.6f %.6f %.6f %.6f %d %s\n", l->src, l->dst, l->weight,
                l->bandwidth_kbps, l->prop_delay_ms, l->loss_rate, l->queue_cap,
                l->state == LINK_UP ? "UP" : "DOWN");
    }
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}

int io_load_topo(Simulator *sim, const char *path)
{
    FILE *fp;
    char line[256];
    char kind[16];
    if (!sim || !path) {
        return -1;
    }
    fp = fopen(path, "r");
    if (!fp) {
        perror(path);
        return -1;
    }
    graph_free(&sim->graph);
    graph_init(&sim->graph);
    sim->path_src = -1;
    sim->path_dst = -1;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || strncmp(line, "NETSIM", 6) == 0) {
            continue;
        }
        if (sscanf(line, "%15s", kind) != 1) {
            continue;
        }
        if (strcmp(kind, "PATH") == 0) {
            sscanf(line, "%*s %d %d", &sim->path_src, &sim->path_dst);
        } else if (strcmp(kind, "NODE") == 0) {
            int id;
            char name[NETSIM_MAX_NAME];
            float x, y;
            char st[8];
            int got = sscanf(line, "%*s %d %31s %f %f %7s", &id, name, &x, &y, st);
            int nid;
            if (got < 4) {
                continue;
            }
            nid = graph_add_node(&sim->graph, x, y);
            if (nid < 0) {
                fclose(fp);
                return -1;
            }
            snprintf(sim->graph.nodes[nid].name, sizeof(sim->graph.nodes[nid].name), "%s",
                     name);
            if (got >= 5 && strcmp(st, "DOWN") == 0) {
                sim->graph.nodes[nid].state = NODE_DOWN;
            }
        } else if (strcmp(kind, "LINK") == 0) {
            int a, b, k;
            double w, bw, dly, loss;
            char st[8];
            int lid;
            int got = sscanf(line, "%*s %d %d %lf %lf %lf %lf %d %7s", &a, &b, &w, &bw,
                             &dly, &loss, &k, st);
            if (got < 7) {
                continue;
            }
            lid = graph_add_link(&sim->graph, a, b, w, bw, dly, loss, k);
            if (lid < 0) {
                fclose(fp);
                return -1;
            }
            if (got >= 8 && strcmp(st, "DOWN") == 0) {
                graph_set_link_state(&sim->graph, lid, LINK_DOWN);
            }
        }
    }
    fclose(fp);
    return sim_recompute_route(sim);
}

int io_export_csv(Simulator *sim, const char *path, const char *scenario)
{
    FILE *fp;
    double avg, mn, mx;
    int i;
    char pbuf[256];
    if (!sim || !path) {
        return -1;
    }
    fp = fopen(path, "w");
    if (!fp) {
        perror(path);
        return -1;
    }
    telemetry_latency_ms(&sim->tel, &avg, &mn, &mx);
    routing_format_path(&sim->route, pbuf, sizeof(pbuf));
    fprintf(fp, "kind,id,src,dst,type,sent,recv,drop,pdr,avg_lat_ms,min_ms,max_ms,");
    fprintf(fp, "throughput_kbps,jitter_ms,cwnd,path,cost,reroute_s,settled,iterations,");
    fprintf(fp, "runtime_us,algo\n");
    fprintf(fp,
            "global,-1,-1,-1,%s,%d,%d,%d,%.4f,%.6f,%.6f,%.6f,%.6f,%.6f,0,%s,%.6f,%.6f,%d,%d,%ld,%s\n",
            scenario ? scenario : "", sim->tel.sent, sim->tel.recv, sim->tel.dropped,
            telemetry_pdr(&sim->tel), avg, mn, mx,
            telemetry_throughput_kbps(&sim->tel, sim->now > 0 ? sim->now : sim->end_time),
            sim->tel.lat_n > 1
                ? (sim->tel.jitter_sum / (double)(sim->tel.lat_n - 1)) * 1000.0
                : 0.0,
            pbuf, netsim_finite(sim->route.cost) ? sim->route.cost : -1.0,
            sim->tel.reroute_time, sim->route.settled, sim->route.iterations,
            sim->route.runtime_us,
            sim->algo == ROUTING_BELLMAN_FORD ? "bellman-ford" : "dijkstra");
    for (i = 0; i < NETSIM_MAX_FLOWS; i++) {
        Flow *f = &sim->flows[i];
        double fpdr, favg, fthr, dur;
        const char *ft;
        if (!f->used) {
            continue;
        }
        fpdr = f->sent ? 100.0 * (double)f->recv / (double)f->sent : 0.0;
        favg = f->lat_n ? (f->lat_sum / (double)f->lat_n) * 1000.0 : 0.0;
        dur = sim->now > 0 ? sim->now : 1.0;
        fthr = (f->bytes_recv * 8.0) / (dur * 1000.0);
        ft = (f->type == FLOW_TCP) ? "tcp" : (f->type == FLOW_BURSTY) ? "bursty"
             : (f->type == FLOW_POISSON)                             ? "poisson"
                                                                     : "cbr";
        fprintf(fp, "flow,%d,%d,%d,%s,%d,%d,%d,%.4f,%.6f,%.6f,%.6f,%.6f,%.6f,%.4f,,,,,\n",
                i, f->src, f->dst, ft, f->sent, f->recv, f->dropped, fpdr, favg,
                f->lat_n ? f->lat_min * 1000.0 : 0.0, f->lat_max * 1000.0, fthr,
                f->lat_n > 1 ? (f->jitter_sum / (double)(f->lat_n - 1)) * 1000.0 : 0.0,
                f->cwnd);
    }
    fprintf(fp, "compare,dijkstra,-1,-1,dijkstra,0,0,0,0,0,0,0,0,0,0,%s,%.6f,-1,%d,0,%ld,dijkstra\n",
            pbuf, sim->cmp_dij.cost, sim->cmp_dij.settled, sim->cmp_dij.runtime_us);
    routing_format_path(&sim->cmp_bf, pbuf, sizeof(pbuf));
    fprintf(fp, "compare,bellman,-1,-1,bellman-ford,0,0,0,0,0,0,0,0,0,0,%s,%.6f,-1,0,%d,%ld,bellman-ford\n",
            pbuf, sim->cmp_bf.cost, sim->cmp_bf.iterations, sim->cmp_bf.runtime_us);
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}
