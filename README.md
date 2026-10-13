# Network Traffic & Routing Simulator

Interactive Computer Networks Lab (BCSE308P) tool. You draw a topology, pick a
routing algorithm, inject traffic, fail links, and watch congestion, drops,
rerouting, and live metrics on one screen.

Pipeline: **TOPOLOGY → ROUTING → TRAFFIC → NETWORK STATE → TELEMETRY → INSIGHT**

Language: **C11** plus **raylib** for the GUI. No Python/JS/C++.

## Install raylib

**Linux (Debian/Ubuntu):**

```bash
sudo apt-get update
sudo apt-get install -y build-essential libraylib-dev pkg-config
```

**Windows (MSYS2 mingw-w64):**

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib make
```

**macOS (Homebrew):**

```bash
brew install raylib
```

## Build

```bash
cd netsim
make          # ./netsim
make test     # graph, heap, Dijkstra, Bellman-Ford, queue, events
make headless-demo
make clean
```

Compile flags: `gcc -std=c11 -Wall -Wextra -Wpedantic -O2`.

## Run

**GUI (everything on one page):**

```bash
./netsim
```

**Headless:**

```bash
./netsim --headless --scenario normal_traffic --seed 1 --out results/normal_traffic.csv
```

## GUI map

The window is the lab dashboard:

| Region | What you see |
|--------|----------------|
| **Canvas (center)** | Nodes, links, weights, queue bars, utilization heatmap (green→red), gold shortest path, moving packets (color = flow), red flash on drop |
| **Top bar** | Start / Pause / Reset / Step, speed 0.1×–10× |
| **Right panel** | Algorithm (Dijkstra / Bellman-Ford), cost mode (weight / hops / dynamic), link editor, flow form, scenario buttons, Save/Load/CSV, compare table, adjacency list + matrix, log |
| **Bottom charts** | Latency, throughput, avg queue, TCP cwnd, PDR bars per flow |
| **Status bar** | sim time, algorithm, flow count, packets in flight, sent/recv/drop, PDR, avg latency |

Green node = path source, orange = destination.

## Controls

- Left-click empty canvas: add node
- Left-click two nodes: add a bidirectional link
- Drag a node: move it
- Click a link: select it and load w / bw / delay / loss / K into the editor (Enter or **Apply** recomputes routes)
- Right-click a node: set path source, then destination
- **Fail/Restore** or key **F**: toggle selected link/node
- Delete / Backspace: remove selection
- Space: start/pause
- F12: `docs/screenshots/shot_N.png`
- Escape: clear selection

## Scenarios

| Name | What it measures |
|------|------------------|
| `demo4` | 4-node square, Dijkstra path 0→1→2 cost 2 |
| `normal_traffic` | Poisson 80 kbps, baseline latency / 100% PDR / M/M/1 check |
| `heavy_traffic` | Overload, queueing delay, tail-drop |
| `link_failure` | Fail 0–1 at t=4 s, reroute onto 0→3→2, rerouting time |
| `high_loss` | 20% loss per hop, PDR |
| `algorithm_compare` | Dijkstra vs Bellman-Ford on 8-node mesh |
| `tcp_vs_udp` | TCP AIMD vs UDP on a bottleneck |
| `stress` | 50-node grid, 12 flows |

Sample topologies: `topologies/4node.topo`, `mesh8.topo`, `ring.topo`.

## How metrics are produced

Discrete-event simulation (min-heap of timestamps). Service time =
`packet_size*8 / bandwidth`, then propagation delay. Each directed link has an
M/M/1/K-style FIFO; overflow is tail-drop. Loss is Bernoulli with `loss_rate`.
Routing tables are rebuilt on weight edits and failures. CSV values are the
same counters printed on exit.

## Screenshots to take

1. `docs/screenshots/shot_1.png` — default GUI after **Normal**, gold path 0→1→2, right-hand panels visible
2. `shot_2.png` — **Heavy** after Start: red heatmap, queue bars, PDR < 100%
3. `shot_3.png` — **Fail** after t=4 s: path 0→3→2, log line with reroute time
4. `shot_4.png` — **TCP/UDP** with cwnd chart moving
5. `shot_5.png` — adjacency matrix + Dijkstra vs Bellman-Ford compare panel

## Reproducing RESULTS.md

```bash
cd netsim
make
mkdir -p results
./netsim --headless --scenario normal_traffic --seed 1 --out results/normal_traffic.csv
./netsim --headless --scenario heavy_traffic --seed 1 --out results/heavy_traffic.csv
./netsim --headless --scenario link_failure --seed 1 --out results/link_failure.csv
./netsim --headless --scenario high_loss --seed 1 --out results/high_loss.csv
./netsim --headless --scenario algorithm_compare --seed 1 --out results/algorithm_compare.csv
./netsim --headless --scenario tcp_vs_udp --seed 1 --out results/tcp_vs_udp.csv
./netsim --headless --scenario stress --seed 1 --out results/stress.csv
```
