# Results

Numbers below come from:

```bash
./netsim --headless --scenario <name> --seed 1 --out results/<name>.csv
```

Host: macOS, `gcc -std=c11 -O2`, seed **1**. Do not invent replacements; re-run the command if you need a new table.

## demo4 (routing only)

| Field | Value |
|-------|-------|
| algorithm | Dijkstra |
| src | 0 |
| dst | 2 |
| path | 0 -> 1 -> 2 |
| cost | 2.000 |
| nodes settled | 4 |
| Bellman-Ford iterations | 2 |
| Bellman-Ford cost | 2.000 |

## 1. normal_traffic

Poisson 80 kbps, 500-byte packets, 8 s, 1000 kbps links.

| Metric | Value |
|--------|-------|
| avg latency (ms) | 28.085 |
| min latency (ms) | 28.000 |
| max latency (ms) | 32.456 |
| throughput (kbps) | 66.000 |
| PDR (%) | 100.00 |
| packets sent | 132 |
| packets received | 132 |
| packets dropped | 0 |
| M/M/1 measured queueing delay (ms) | 0.0855 |
| M/M/1 theory queueing delay (ms) | 0.3478 |

Path `0 -> 1 -> 2`, cost 2. Two-hop propagation is 20 ms plus serialization; measured E2E ~28 ms.

## 2. heavy_traffic

Three flows on 200 kbps links, K=8, 8 s.

| Metric | Value |
|--------|-------|
| avg latency (ms) | 206.406 |
| throughput (kbps) | 191.500 |
| PDR (%) | 56.49 |
| packets sent | 678 |
| packets received | 383 |
| packets dropped | 295 |
| avg queueing delay (ms) | 146.406 |

Queue buildup and tail-drop are visible: delay jumps from ~28 ms (light load) to ~206 ms.

## 3. link_failure

CBR 120 kbps; link 0–1 fails at t=4 s.

| Metric | Value |
|--------|-------|
| failure time (s) | 4.000 |
| rerouting time (s) | 0.063333 |
| old path | 0 -> 1 -> 2 (cost 2) |
| new path | 0 -> 3 -> 2 (cost 5) |
| packets sent | 286 |
| packets received | 285 |
| packets dropped on fail | 1 |
| PDR (%) | 99.65 |

## 4. high_loss

20% loss on every directed link, 2-hop path, CBR 100 kbps.

| Metric | Value |
|--------|-------|
| configured loss | 0.20 per link |
| PDR (%) | 60.64 |
| packets sent | 188 |
| packets received | 114 |
| packets dropped | 74 |

Independent per-hop loss predicts about \(0.8^2 = 64\%\) delivery; measured 60.64%.

## 5. algorithm_compare

8-node mesh (`topologies/mesh8.topo`), src=0 dst=7.

| Algorithm | cost | path | iterations / settled | runtime_us |
|-----------|------|------|----------------------|------------|
| Dijkstra | 4.000 | 0 -> 4 -> 5 -> 6 -> 7 | settled 8 | 0 |
| Bellman-Ford | 4.000 | 0 -> 4 -> 5 -> 6 -> 7 | iter 2 | 1 |

Costs match on this positive-weight graph.

## 6. tcp_vs_udp

TCP and UDP/CBR sharing 250 kbps links, 8 s.

| Flow | type | sent | recv | drop | PDR (%) | avg latency (ms) | throughput (kbps) |
|------|------|------|------|------|---------|------------------|-------------------|
| 0 | TCP | 369 | 193 | 175 | 52.30 | 262.323 | 99.571 |
| 1 | UDP/CBR | 338 | 292 | 46 | 86.39 | 267.826 | 151.715 |
| global | | 707 | 485 | 221 | 68.60 | 265.637 | 242.500 |

UDP does not back off, so TCP gets a smaller share and more drops.

## Stress (50-node grid, 12 flows, 3 s)

| Metric | Value |
|--------|-------|
| nodes | 50 |
| path 0→49 cost | 13 |
| packets sent | 432 |
| packets received | 432 |
| PDR (%) | 100.00 |
| Dijkstra settled | 50 |
| Dijkstra runtime_us | 3 |
| Bellman-Ford iterations | 2 |
| Bellman-Ford runtime_us | 4 |
