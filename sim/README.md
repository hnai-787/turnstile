# schedsim — CPU Scheduling Simulation & Historical-Kernel Validation Framework

This is the companion simulation framework for the workspace-root
[Round-Robin Linux Kernel Scheduler](../README.md) project. That project
recompiled a real Linux kernel with a patched round-robin scheduler and
measured it against the stock kernel — real, one-off, unrepeatable work.
**schedsim** is the reusable half: a discrete-event simulator implementing
four CPU scheduling algorithms (round-robin, fixed-priority, MLFQ, and a
classic-CFS approximation) over configurable synthetic workloads, whose
distinguishing feature — unlike most classroom scheduler simulators — is
that it can be checked against the real measured kernel data already
sitting in [`../validation/kernel-6.12.25/`](../validation/kernel-6.12.25/).

## Design decisions

**Why a discrete-event simulator, not a tick loop.** The simulator always
jumps directly to the next relevant event (a burst/quantum boundary, an
arrival, or an I/O completion) rather than stepping through fixed time
units. This is exact (no resolution artifacts), fast for large workloads,
and standard practice for this class of simulator (the official OSTEP
`mlfq.py` teaching simulator takes the same approach).

**Why "cfs-lite," not "CFS," and definitely not "Linux 6.12."** Linux
began transitioning its fair scheduling class from classic CFS to
**EEVDF** (Earliest Eligible Virtual Deadline First) starting in kernel
6.6 — a 6.12 kernel's `fair.c` still maintains weighted vruntime, but task
selection uses EEVDF eligibility and virtual deadlines (`pick_eevdf`), not
the classic "always pick the minimum-vruntime task" rule. `cfs-lite` here
implements *that* classic rule (weighted vruntime, minimum selection,
using Linux's real 40-entry nice-to-weight table) as a clean, pedagogical
approximation — explicitly not a claim to model 6.12's actual EEVDF
behavior. See [`../validation/kernel-6.12.25/README.md`](../validation/kernel-6.12.25/README.md)
for the full correction (the original project's README called the
baseline "CFS," which needed this clarification).

**Why MLFQ's constants are a named preset, not "the" MLFQ spec.** OSTEP is
explicit that queue count, quantum sizes, allotments, and boost interval
are workload-dependent tuning parameters with no universal answer —
Solaris historically used ~60 queues, for instance. `MlfqConfig::ostepDemo()`
provides one illustrative, citable starting point (matching the shape of
OSTEP's own worked example: shorter quanta at higher priority), not a
standard.

**Rule 4 (anti-gaming) is implemented precisely.** A task that yields for
I/O repeatedly, always just under its level's quantum, must still be
demoted once its *cumulative* CPU consumption at that level reaches the
level's allotment — not be granted a fresh allotment by each yield. This
is directly tested (`tests/test_policies.cpp` and `tests/test_invariants.cpp`)
by simulating exactly that gaming strategy and confirming demotion still
occurs.

**A real bug this design caught:** an early version of the discrete-event
loop only checked "was this the task's last burst?" when a *CPU* burst
ended, not when an *I/O* burst ended. Any workload whose last burst was
I/O (a perfectly normal shape — a task that computes, then waits on a
final write, then exits) triggered an out-of-bounds burst-array access:
undefined behavior that surfaced as a corrupted, garbage `completion`
timestamp. Caught immediately by this project's own test suite (an MLFQ
anti-gaming workload happened to end in I/O) rather than by inspection.
Fixed by checking burst-array exhaustion in both places the discrete
-event loop can retire a task's last burst.

**A related second bug, also test-caught:** `Task::id` and
`Task::arrival` had no default member initializers, so a hand-built
`Task{}` that forgot to set `.arrival` silently picked up indeterminate
stack memory — which is exactly what one of this project's own test
workloads did, producing a `completion` value that was, on inspection, a
stack address. Fixed by giving every `Task` field a safe default (`= 0`),
so the failure mode for a forgotten field is "task arrives at time 0,"
not memory corruption.

**Why raw CPU/file-I/O throughput are marked `not_comparable` against the
real kernel data, on principle, not just by observed result.** In a
zero-overhead discrete-event simulator, every scheduling policy completes
the same total CPU work in the same total busy time *by construction* —
there is no mechanism by which switching from RR to CFS-lite could change
aggregate throughput here. The real kernel's throughput difference
necessarily comes from effects this simulator doesn't model (context
-switch cost, cache behavior, accounting overhead, migration). Claiming
otherwise would mean the simulator was quietly modeling something it
isn't. See `schedsim validate`'s output and `src/validation.cpp`.

**Why one comparison came back `discordant`, and why that was kept.**
`schedsim validate` compares simulated scheduling-delay direction against
the real kernel's `perf sched latency` result, using the `interactive-mix`
preset. The real kernel showed round-robin with *lower* total scheduling
delay than the fair scheduler; the simulator shows the opposite for this
workload (RR's 100-unit quantum causes long queuing behind the workload's
CPU-bound hog task before short interactive tasks get a turn, while
`cfs-lite`'s finer re-evaluation granularity avoids that). Per this
project's own validation methodology (see "Historical validation" below),
a disagreement on a `QUALITATIVE_ONLY` row is reported honestly as
`discordant`, with a real hypothesis for why, rather than retuned until it
happens to agree — see `src/validation.cpp` and the live
`schedsim validate` output quoted below.

## Algorithms

| Name | Policy | Preemption |
|---|---|---|
| `rr` | Round-robin, fixed quantum | Never on arrival; relies on quantum expiry |
| `priority` | Fixed priority, FIFO/round-robin within a priority, optional aging | On arrival of a higher-priority task |
| `mlfq` | OSTEP's 5 rules: highest queue wins, RR within a queue, new jobs enter the top queue, allotment-based demotion (cumulative across yields), periodic boost | On arrival into a higher queue |
| `cfs-lite` | Weighted vruntime, always run the ready task with minimum vruntime, Linux's real nice-to-weight table | Never on arrival; relies on periodic granularity re-evaluation |

## Metrics

Per task: turnaround, response, ready-wait, CPU service, I/O-blocked time,
max dispatch delay (mirroring `perf sched latency`'s "max delay" column),
dispatch count, preemption count, slowdown/stretch. System-level:
utilization, throughput, context switches, preemptions, and an unweighted
Jain fairness index over CPU service time (appropriate for equal-priority
workloads — see "Limitations" for why a nice-mix workload is instead
validated via a direct proportional-share check, not folded into this
single number).

## Historical validation

Real, freshly-computed output from `schedsim validate` (not a
pre-recorded verdict — every run recomputes it):

```text
[not_comparable] CPU throughput (sysbench events/s; stress-ng bogo-ops/s)
  kernel direction:     fair_better
  simulation direction: not_modeled

[not_comparable] File I/O throughput (sysbench fileio read)
  kernel direction:     rr_better
  simulation direction: not_modeled

[discordant] Scheduling delay (perf sched latency total vs. simulated avg max-dispatch-delay, interactive-mix workload)
  kernel direction:     rr_better
  simulation direction: fair_better
  Simulated: RR=81.000, cfs-lite=10.500.

[concordant] CPU-share fairness (top snapshot CPU% distribution vs. Jain fairness index, cpu-bound-equal workload)
  kernel direction:     rr_better
  simulation direction: tie
  Simulated: RR Jain index=1.00000, cfs-lite Jain index=1.00000.
```

Honest summary: 2 of 4 comparisons are not meaningfully comparable at all
(platform/hardware throughput effects this simulator doesn't model), 1 is
concordant (both show RR at least as fair, though the simulator's
idealized equal-weight case can't show the *skew* the real kernel
exhibited), and 1 is genuinely discordant with a documented hypothesis for
why. This is the outcome you get from an honest methodology — not a
pre-written result.

## Verified proportional-share example (real, captured output)

`nice-mix` (three tasks, equal 3000-unit CPU demand, nice -10/0/10),
under `cfs-lite`:

```text
$ schedsim run nice-mix --algo cfs-lite --format json
task 1 (nice -10): completion=3360
task 2 (nice   0): completion=6324
task 3 (nice  10): completion=9000
```

This matches the theoretical proportional-share calculation almost
exactly: with weights 9548/1024/110 (Linux's real nice-to-weight table),
task 1's expected completion while sharing with two lower-weight
competitors is `3000 / (9548/10682) ≈ 3356` — the simulator gives 3360.

## Repository Structure (this subdirectory)

```text
sim/
  README.md (this file), CMakeLists.txt, build.sh
  include/schedsim/   types, policy interface, the 4 policies, simulator, metrics, report, validation, workload I/O/presets
  src/                implementations + main.cpp (CLI)
  tests/              24 Catch2 test cases
  workloads/          (reserved for user-authored workload JSON files)
```

See the [workspace-root README](../README.md) for the original kernel
patch project this framework complements, and
[`../validation/kernel-6.12.25/`](../validation/kernel-6.12.25/) for the
immutable real measurement data.

## Building from source

Requires CMake, MinGW g++, and the vcpkg instance already set up for the
other C++ projects in this workspace:

```bash
./build.sh
```

## Usage

```bash
schedsim run <workload.json|preset> --algo rr|priority|mlfq|cfs-lite [--quantum N] [--format text|json]
schedsim compare <workload.json|preset> [--algos rr,priority,mlfq,cfs-lite]
schedsim validate
schedsim presets
schedsim export <preset> -o <file.json>
```

Presets: `cpu-bound-equal`, `cpu-bound-mixed`, `interactive-mix`,
`nice-mix`, `starvation`.

## Testing

```text
$ ./build/schedsim_tests.exe
All tests passed (133 assertions in 24 test cases)
```

Coverage: Jain index edge cases; per-policy behavior (RR ordering, fixed
-priority preemption and aging, MLFQ's exact anti-gaming Rule 4 and
periodic boost, CfsLite's real nice-weight table and min-vruntime
selection); end-to-end simulator correctness (single task, RR
interleaving, I/O round-trip, late arrivals, priority preemption, MLFQ
demotion dispatch-count arithmetic, CfsLite equal-weight symmetry); and
cross-cutting invariants (CPU conservation, exact CPU-demand
satisfaction, RR non-starvation, CfsLite weighted proportional share)
checked across every policy and every workload preset.

## Limitations

- **Fairness metric is unweighted Jain**, appropriate for equal-priority
  workloads; deliberately-unequal (nice-mix) scenarios are validated via
  a direct proportional-share completion-order check instead (see
  "Verified proportional-share example"), not folded into one fairness number.
- **No CPU/context-switch overhead modeled** — this is precisely why raw
  throughput comparisons against the real kernel are marked
  `not_comparable` rather than attempted.
- **No multicore, no NUMA, no cache modeling.**
- **CLI exposes only the RR quantum**, not per-algorithm tuning (aging
  config, MLFQ levels/quanta, CfsLite granularity) — those are set via
  the C++ `AlgoSpec` API; wiring more flags through the CLI is future work.
- **No EEVDF-lite policy yet** — `cfs-lite` approximates classic CFS only,
  not Linux 6.12's actual EEVDF-era fair class (by design; see "Design decisions").
- **The scheduling-delay historical comparison is discordant** for the
  `interactive-mix` workload (documented above and in `src/validation.cpp`),
  not concordant — reported honestly rather than adjusted.
- **No CI pipeline has run against this code** — not pushed to GitHub in
  this task.

## Future Enhancements

- An `eevdf-lite` policy (virtual deadline + lag/eligibility) for a
  genuinely closer approximation of Linux 6.12's actual fair class.
- Entitlement-normalized Jain fairness (weight-aware), per the research
  this was built against, instead of the current unweighted index plus a
  separate proportional-share check.
- CLI flags for per-algorithm tuning (aging, MLFQ levels, CfsLite granularity).
- CSV export for spreadsheet-based comparison across many workload/algorithm combinations.
- A wider workload corpus with randomized/generated bursts for broader property-test coverage.

## Safety and Privacy

No secrets, credentials, or private data are involved. All workloads are
synthetic.

## Ethical Notice

Academic/portfolio exercise; the simulator performs only in-memory,
synthetic computation. No ethical concerns apply.
