# Round-Robin Linux Kernel Scheduler

## Course Information

| Field | Details |
|---|---|
| Course | Operating Systems Lab (CS-325L) |
| Semester | Semester 4 — Spring 2025 |
| University | Air University, Islamabad |
| Students | Hussain Ali (232095), Syed Jazib Ali Rizvi (232145), Sardar Ahmad Ali (232147), Sardar Shahbaz (232089) |

## Overview

Recompiles the Linux kernel (6.12.25, Kali Rolling 2025.2), replacing the
Completely Fair Scheduler (CFS) in `kernel/sched/fair.c` with a simplified
round-robin scheduler, then benchmarks the modified kernel against vanilla
CFS to compare fairness and throughput trade-offs.

## Problem Statement

CFS is the Linux default for good reason — but a round-robin scheduler is
simpler and more deterministic. This project asks: what do you actually
give up (and gain) by making that swap, measured rather than assumed?

## Objectives

- Replace CFS's core scheduling functions with a round-robin implementation.
- Benchmark both kernels under identical CPU, file I/O, and scheduling-latency workloads.
- Report the real trade-offs rather than declaring a single "winner."

## Tools and Technologies

- Linux kernel source (C), gcc/make
- sysbench, stress-ng, `perf sched latency`
- Python (`matplotlib`, `argparse`) for orchestration and reporting
- bash

## Features

- A working round-robin scheduler patch (`RR_TIMESLICE` = 100ms) replacing
  `pick_next_task_fair` / `enqueue_task_fair` / `dequeue_task_fair` /
  `task_tick_fair`, with CFS's vruntime/red-black-tree logic removed.
- A benchmark harness (`benchmarks/kernelTest.py`, `--baseline`/`--new`/`--compare`)
  running sysbench CPU/fileio, stress-ng, `perf sched latency`, and a `top` snapshot under load.
- An HTML comparison report generator.

## Methodology

1. Build and boot the vanilla kernel; capture baseline benchmark results.
2. Apply the round-robin patch (`scheduler-source/fair_modified.c`, diffed against `fair_original.c`); rebuild and reboot.
3. Re-run the identical benchmark suite.
4. Compare results per metric rather than relying solely on the harness's naive scoring.

## Repository Structure

```text
round-robin-kernel-scheduler/
  README.md
  PROJECT_NOTES.md
  scheduler-source/
    fair_original.c
    fair_modified.c
  benchmarks/
    kernel_baseline_test.sh
    kernelTest.py
    evaluate.c
  results/
    kernel_roundrobin/  (kernel_info, perf_sched_latency, stressng_cpu, sysbench_cpu, sysbench_fileio, top_snapshot, yes_pids)
    kernel_vanilla/     (same set)
    kernel_comparison_report.html
  docs/kernel-round-robin-scheduler-report.docx (+ .pdf)
  screenshots/
  project.yaml
```

## Setup Instructions

This modifies a running kernel — **only do this in a disposable VM**, never
on a primary machine.

```bash
# inside a Linux kernel source tree, after applying the diff between
# fair_original.c and fair_modified.c to kernel/sched/fair.c
make -j$(nproc)
make modules_install && make install
reboot
```

## Usage

```bash
sudo bash benchmarks/kernel_baseline_test.sh   # run on each kernel (vanilla, then round-robin)
python benchmarks/kernelTest.py --compare      # generate results/kernel_comparison_report.html
```

## How to Review

1. Start with this README, then the human-written report in `docs/`.
2. Diff `scheduler-source/fair_original.c` against `fair_modified.c` to see exactly what changed.
3. Read the raw results under `results/kernel_roundrobin/` and `results/kernel_vanilla/`.
4. Treat `results/kernel_comparison_report.html`'s "winner" line as a simple sum-of-metrics heuristic, not a statistically rigorous verdict — see Results below for the more nuanced read.

## Screenshots

See `screenshots/` — 8 screenshots covering the build/boot process and benchmark runs.

## Results

Real, measured numbers from `results/kernel_roundrobin/` and `results/kernel_vanilla/`:

| Test | Vanilla (CFS) | Round-Robin | Better |
|---|---|---|---|
| sysbench CPU (events/s) | 93.04 | 76.82 | CFS |
| stress-ng CPU (ops/s) | 4247 | 4194 | ~tie |
| `perf sched latency` (total) | 1119.708 ms | 896.388 ms | Round-Robin |
| sysbench fileio (read) | 11.34 MiB/s | 15.37 MiB/s | Round-Robin |
| CPU% fairness (`top` snapshot) | skewed | more uniform | Round-Robin |

The auto-generated `kernel_comparison_report.html` sums these into a crude
"Winner: Modified Kernel (3 vs 1)" — that heuristic is naive. The
human-written report's actual conclusion is more accurate: round-robin
offers fairness and deterministic scheduling latency, while **CFS remains
superior for general-purpose computing** due to adaptive prioritization and
more efficient time-sharing under CPU-bound load.

## Limitations

- Coursework-level prototype — not tuned or hardened for production use.
- Benchmarked on a single VM configuration; results may vary by hardware and workload mix.
- Round-robin implementation is intentionally simplified (no priority levels, no load balancing across cores).

## Future Enhancements

- Multi-core-aware load balancing for the round-robin queue.
- A statistically rigorous comparison (multiple runs, confidence intervals) instead of single-run benchmarks.

## Safety and Privacy

- No secrets, credentials, or private data are involved.
- This modifies kernel scheduling behavior — test only in an isolated VM, never on a system relied on for other work.

## Ethical Notice

Academic coursework exercise; no ethical concerns apply. The safety note
above is a technical caution (a bad kernel scheduler patch can make a
machine unresponsive), not an ethics concern.
