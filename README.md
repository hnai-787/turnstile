# Round-Robin Linux Kernel Scheduler + schedsim (Scheduler Simulation Framework)

## Course Information

| Field | Details |
|---|---|
| Course | Operating Systems Lab (CS-325L) |
| Semester | Semester 4 — Spring 2025 |
| University | Air University, Islamabad |
| Students | Hussain Ali (232095), Syed Jazib Ali Rizvi (232145), Sardar Ahmad Ali (232147), Sardar Shahbaz (232089) |

## Overview

This project has two parts now. The **original, unmodified work**
(preserved under `archive/academic-original/`) recompiled the Linux
kernel (6.12.25, Kali Rolling 2025.2), replacing the fair scheduling
class in `kernel/sched/fair.c` with a simplified round-robin scheduler,
then benchmarked the modified kernel against the unmodified one. That's
real, one-off, unrepeatable work — recompiling and rebooting a kernel
isn't something a normal dev session can redo. The **new addition**,
[`sim/`](sim/), is a reusable **userspace scheduling-algorithm simulation
framework** (round-robin, fixed-priority, MLFQ, and a classic-CFS
approximation) — and, uniquely among this workspace's rebuilt projects,
it can be checked against the *real measured kernel data* preserved under
[`validation/kernel-6.12.25/`](validation/kernel-6.12.25/), rather than
having no empirical grounding at all.

## Problem Statement

The fair scheduling class is the Linux default for good reason — but a
round-robin scheduler is simpler and more deterministic. This project
asks: what do you actually give up (and gain) by making that swap,
measured rather than assumed? And, since that measurement can't be
repeated on demand: what *can* be usefully explored in a fast, reusable
simulator, and how honestly can that simulator's findings be checked
against the one real measurement that exists?

## A factual correction from this rebuild

The original README and report called the unmodified baseline "CFS" (the
Completely Fair Scheduler). Linux began transitioning the fair scheduling
class from classic CFS to **EEVDF** (Earliest Eligible Virtual Deadline
First) starting in kernel 6.6; a 6.12 kernel's `fair.c` still maintains
weighted vruntime, but task selection uses EEVDF eligibility and virtual
deadlines (`pick_eevdf`), not the classic "always pick minimum vruntime"
rule. The real measured data is completely unaffected by this — only the
baseline's *name* needed correcting. See
[`validation/kernel-6.12.25/README.md`](validation/kernel-6.12.25/README.md)
for the full explanation, and `sim/README.md` "Design decisions" for why
the simulator's `cfs-lite` policy is explicitly a classic-CFS
approximation, not a claim to model 6.12's actual EEVDF behavior.

## Objectives

- Replace the fair scheduler's core scheduling functions with a round-robin implementation. *(original work)*
- Benchmark both kernels under identical CPU, file I/O, and scheduling-latency workloads. *(original work)*
- Report the real trade-offs rather than declaring a single "winner." *(original work)*
- Build a reusable, general scheduling-algorithm simulator and check its
  scheduler-level findings against the real kernel measurement, honestly
  reporting agreement *and* disagreement. *(this rebuild)*

## Tools and Technologies

- Linux kernel source (C), gcc/make; sysbench, stress-ng, `perf sched
  latency`; Python/bash *(original benchmarking work — see `archive/academic-original/`)*
- C++20, CMake, Catch2 *(the new `sim/` simulation framework)*

## Features

- A working round-robin scheduler patch (`RR_TIMESLICE` = 100ms) replacing
  `pick_next_task_fair` / `enqueue_task_fair` / `dequeue_task_fair` /
  `task_tick_fair`, with the fair class's vruntime/red-black-tree logic removed.
- A benchmark harness (`archive/academic-original/benchmarks/kernelTest.py`)
  running sysbench CPU/fileio, stress-ng, `perf sched latency`, and a `top` snapshot under load.
- **New:** `sim/`, a discrete-event simulator implementing round-robin,
  fixed-priority (with aging), OSTEP-style MLFQ (with the exact
  anti-gaming Rule 4), and a classic-CFS approximation (Linux's real
  nice-to-weight table), with response/turnaround/fairness/scheduling
  -delay metrics and a `schedsim validate` command that compares
  simulated scheduler-level trends against the real kernel data.

## Methodology

1. Build and boot the vanilla kernel; capture baseline benchmark results. *(original)*
2. Apply the round-robin patch; rebuild and reboot. *(original)*
3. Re-run the identical benchmark suite. *(original)*
4. Compare results per metric rather than relying solely on the harness's naive scoring. *(original)*
5. **New:** preserve that real data as immutable historical reference
   (`validation/kernel-6.12.25/`), build a general simulator, and validate
   its scheduler-level (not platform-level) findings against it using an
   explicit three-tier methodology (direct / qualitative-direction-only /
   not-comparable) — see `sim/README.md` "Historical validation".

## Repository Structure

```text
round-robin-kernel-scheduler/
  README.md, PROJECT_NOTES.md, CHANGELOG.md, project.yaml
  sim/                          NEW: the C++ scheduling simulation framework (see sim/README.md)
  validation/kernel-6.12.25/    NEW: immutable real kernel measurement + provenance manifest
  archive/academic-original/    the original patch, benchmark scripts, and report, untouched
    scheduler-source/  fair_original.c, fair_modified.c
    benchmarks/        kernel_baseline_test.sh, kernelTest.py, evaluate.c
    docs/              kernel-round-robin-scheduler-report.docx/.pdf
  results/                       original raw benchmark output (still present; also copied into validation/)
  screenshots/
```

## Setup Instructions (original kernel patch)

This modifies a running kernel — **only do this in a disposable VM**, never
on a primary machine.

```bash
# inside a Linux kernel source tree, after applying the diff between
# archive/academic-original/scheduler-source/fair_original.c and fair_modified.c
# to kernel/sched/fair.c
make -j$(nproc)
make modules_install && make install
reboot
```

## Usage

Original kernel benchmarking:

```bash
sudo bash archive/academic-original/benchmarks/kernel_baseline_test.sh
python archive/academic-original/benchmarks/kernelTest.py --compare
```

New simulation framework (see `sim/README.md` for full usage):

```bash
cd sim && ./build.sh
./build/schedsim.exe compare nice-mix
./build/schedsim.exe validate
```

## How to Review

1. Start with this README, then `sim/README.md` for the new framework.
2. Diff `archive/academic-original/scheduler-source/fair_original.c`
   against `fair_modified.c` to see exactly what changed in the real kernel.
3. Read the raw historical results under `validation/kernel-6.12.25/`.
4. Run `schedsim validate` (in `sim/`) and compare its output against the
   "Historical validation" section of `sim/README.md` — it's real,
   freshly-computed output, not a fixed report.

## Screenshots

See `screenshots/` — 8 screenshots covering the original build/boot process and benchmark runs.

## Results

### Original kernel measurement (real, unchanged)

| Test | Vanilla (fair scheduler) | Round-Robin | Better |
|---|---|---|---|
| sysbench CPU (events/s) | 93.04 | 76.82 | fair scheduler |
| stress-ng CPU (ops/s) | 4247 | 4194 | ~tie |
| `perf sched latency` (total) | 1119.708 ms | 896.388 ms | Round-Robin |
| sysbench fileio (read) | 11.34 MiB/s | 15.37 MiB/s | Round-Robin |
| CPU% fairness (`top` snapshot) | skewed | more uniform | Round-Robin |

The auto-generated `kernel_comparison_report.html` sums these into a crude
"Winner: Modified Kernel (3 vs 1)" — that heuristic is naive. The
human-written report's actual conclusion is more accurate: round-robin
offers fairness and deterministic scheduling latency, while the **fair
scheduler remains superior for general-purpose computing** due to
adaptive prioritization and more efficient time-sharing under CPU-bound load.

### Simulation vs. historical kernel data (new, real, freshly re-run every time)

```text
$ cd sim && ./build/schedsim.exe validate
[not_comparable] CPU throughput            -- kernel: fair scheduler better; simulator: not modeled
[not_comparable] File I/O throughput        -- kernel: RR better; simulator: not modeled
[discordant]     Scheduling delay           -- kernel: RR better; simulator: fair-scheduler-approx better
[concordant]     CPU-share fairness         -- kernel: RR better; simulator: tie (both near-perfect)
```

Honest summary, not a forced match: raw throughput is correctly marked
not comparable (the simulator has no notion of context-switch cost or
hardware effects — see `sim/README.md`). The fairness direction is
concordant. The scheduling-delay direction is **discordant** — the
simulator's `cfs-lite` shows *lower* interactive scheduling delay than
round-robin for the `interactive-mix` workload, the opposite of what the
real kernel showed. `sim/README.md` "Design decisions" documents the
actual hypothesis (RR's 100-unit quantum causes long queuing behind a
CPU-bound task in that specific synthetic workload) rather than retuning
parameters until it agreed, per the research this rebuild was grounded in.

## Limitations

- Coursework-level prototype — not tuned or hardened for production use.
- Benchmarked on a single VM configuration, one run, no repetitions or
  confidence intervals; results may vary by hardware and workload mix.
  See `validation/kernel-6.12.25/environment.json` for the full,
  explicit list of caveats on the real measurement.
- Round-robin implementation is intentionally simplified (no priority
  levels, no load balancing across cores).
- The simulator has no notion of hardware/platform overhead, so it
  cannot and does not attempt to reproduce the real kernel's raw
  throughput results (see `sim/README.md`).

## Future Enhancements

- Multi-core-aware load balancing for the round-robin queue (kernel side).
- A statistically rigorous comparison (multiple runs, confidence
  intervals) instead of single-run kernel benchmarks.
- See `sim/README.md` "Future Enhancements" for the simulator's own roadmap
  (an `eevdf-lite` policy, entitlement-normalized fairness, CLI tuning flags).

## Safety and Privacy

- No secrets, credentials, or private data are involved.
- The original kernel patch modifies scheduling behavior — test only in
  an isolated VM, never on a system relied on for other work. The new
  `sim/` framework is a pure userspace program with no such risk.

## Ethical Notice

Academic coursework exercise; no ethical concerns apply. The safety note
above is a technical caution (a bad kernel scheduler patch can make a
machine unresponsive), not an ethics concern.
