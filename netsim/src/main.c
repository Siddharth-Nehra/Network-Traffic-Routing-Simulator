#include "scenarios.h"
#include "sim.h"
#include "ui_canvas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Print CLI help. */
static void print_usage(const char *argv0)
{
    printf("Usage: %s [--headless] [--scenario NAME] [--seed N] [--out FILE.csv]\n",
           argv0);
    printf("  GUI (default): topology, routing, traffic, charts, metrics\n");
    printf("  --headless     run without a window\n");
    printf("  --scenario     demo4 | normal_traffic | heavy_traffic | link_failure |\n");
    printf("                 high_loss | algorithm_compare | tcp_vs_udp | stress\n");
    printf("  --seed N       RNG seed (default 1)\n");
    printf("  --out FILE     write metrics CSV in headless mode\n");
}

/* Parse --headless/--scenario/--seed/--out. Returns 1 for --help. */
static int parse_cli(int argc, char **argv, int *headless, const char **scenario,
                     uint32_t *seed, const char **out_csv)
{
    int i;
    *headless = 0;
    *scenario = "normal_traffic";
    *seed = 1u;
    *out_csv = NULL;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            *headless = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            *scenario = argv[++i];
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            *seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            *out_csv = argv[++i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int headless = 0;
    const char *scenario = "normal_traffic";
    uint32_t seed = 1u;
    const char *out_csv = NULL;
    Simulator *sim;
    int rc;
    int parse_rc = parse_cli(argc, argv, &headless, &scenario, &seed, &out_csv);
    if (parse_rc > 0) {
        return 0;
    }
    if (parse_rc < 0) {
        return 2;
    }
    sim = sim_create(seed);
    if (!sim) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    if (headless) {
        rc = scenario_run_headless(sim, scenario, out_csv);
        sim_destroy(sim);
        return rc;
    }
    if (scenario && strcmp(scenario, "normal_traffic") != 0) {
        scenario_load(sim, scenario);
    }
    rc = ui_canvas_loop(sim);
    telemetry_print_summary(sim, sim->ui.scenario[0] ? sim->ui.scenario : "gui");
    if (rc != 0 && sim->last_error[0]) {
        fprintf(stderr, "%s\n", sim->last_error);
    }
    sim_destroy(sim);
    return rc == 0 ? 0 : 1;
}
