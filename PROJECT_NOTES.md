# Project Notes

## Source

Migrated from `air-university-cybersecurity-projects/projects/round-robin-kernel-scheduler`
into this workspace as an independent project on 2026-09-07.

## Cleanup decisions

- No build output/binaries existed in the source folder; copied as-is.

## Assumptions

- Benchmark numbers in the README are taken directly from the real
  `results/kernel_roundrobin/*.txt` and `results/kernel_vanilla/*.txt`
  files — none were estimated or invented.
- The auto-generated HTML report's "winner" framing was deliberately
  softened in the README in favor of the human-written report's more
  nuanced conclusion, since the HTML heuristic (sum of metrics) overstates
  round-robin's advantage.
- Group members in the course-info table are taken from the docx/PDF report.

## Remaining work

- None identified — this was the most rigorously documented project in the
  source set (real diffed kernel source, real benchmark data, a written
  conclusion that appropriately hedges the naive auto-generated one).

## 2026-09-08: Added schedsim (scheduling simulation & validation framework)

### What changed and why

This project was already the most rigorously done in the migrated set
(real kernel patch, real measured data, an honest written conclusion), so
the enhancement direction was additive rather than corrective: build the
reusable userspace scheduling-algorithm simulator the original task
suggestion called for (round-robin, priority, MLFQ, a CFS approximation),
and use the unique asset this project already has -- real kernel
measurement data -- to validate it, rather than leaving the simulator
with zero empirical grounding like most classroom scheduler simulators.

The original patch, benchmark scripts, and report were archived unmodified
under `archive/academic-original/`; the real measured results were copied
(not moved) into `validation/kernel-6.12.25/` as an explicitly immutable
historical reference, separate from anything the simulator generates.

### Key engineering decisions and why

- **A factual correction, not just an enhancement.** Research surfaced
  that Linux's fair scheduling class moved from classic CFS to EEVDF
  starting in kernel 6.6 -- a 6.12 baseline is EEVDF-era, not "CFS" as
  both the original README and report called it. This doesn't change
  anything about what was actually measured; it changes how the baseline
  should be *named*. Documented explicitly in
  `validation/kernel-6.12.25/README.md` and the root README, and the
  simulator's classic-CFS-approximation policy is named `cfs-lite` (not
  `cfs`) specifically to avoid re-making the same claim.
- **Discrete-event simulation, not a tick loop** -- exact, fast, and
  matches how the official OSTEP teaching simulator (`mlfq.py`) is built.
- **MLFQ implements OSTEP's Rule 4 precisely**: allotment is consumed
  cumulatively across yields, not reset by them, specifically to prevent
  the classic gaming strategy (yield for I/O just before your quantum
  expires to stay at a high priority forever). Directly tested by
  simulating exactly that gaming strategy.
- **`cfs-lite` uses Linux's real 40-entry nice-to-weight table**, not an
  invented approximation like `pow(1.25, nice)` -- verified against
  known anchor values (nice 0 = 1024, nice -20 = 88761, etc.) and,
  end-to-end, against a hand-computed theoretical proportional-share
  calculation (see "Verification performed" below).
- **Historical validation uses an explicit three-tier methodology**
  (direct / qualitative-direction-only / not-comparable) rather than
  comparing absolute numbers between two fundamentally different
  measurement systems (an idealized zero-overhead simulator vs. a real
  kernel in a VM with host noise). Raw CPU/file-I/O throughput are marked
  not-comparable *on principle* (a zero-overhead simulator cannot show a
  throughput difference between scheduling policies by construction, not
  just because this implementation doesn't happen to model it).

### Two real bugs found and fixed via the test suite

1. The discrete-event loop's I/O-completion handler (`bringInIo`)
   incremented a task's burst index and unconditionally called
   `onReady`, without ever checking whether that was the task's *last*
   burst. A workload ending in an I/O burst (a normal shape -- compute,
   then wait on a final write, then exit) would silently walk off the end
   of the burst array on the next dispatch attempt: undefined behavior,
   which surfaced as a garbage (stack-address-shaped) `completion`
   timestamp in a test. Fixed by adding the same "was this the last
   burst?" check that the CPU-burst-ending code path already had.
2. `Task::id` and `Task::arrival` had no default member initializers.
   One of this project's own hand-built test workloads forgot to set
   `.arrival`, silently picking up indeterminate stack memory and
   producing a nonsensical `completion` value. Fixed at the type level
   (every `Task` field now defaults to `0`) rather than only in the test,
   since any future caller of the library could make the identical mistake.

### Verification performed

`cmake --build` and the full test suite (24 test cases / 133 assertions,
all passing) were actually run. `schedsim validate` was actually executed
against the real historical data and its output is quoted verbatim in
`sim/README.md` and the root README -- including the one discordant
finding, reported as-is rather than adjusted. The `cfs-lite` proportional
-share behavior was independently checked by hand: for the `nice-mix`
workload (weights 9548/1024/110), the theoretical completion time for the
highest-weight task while sharing with the other two is `3000 /
(9548/10682) ≈ 3356` time units; the simulator produced 3360 -- a ~0.1%
match, confirming the weighted-vruntime implementation is mathematically
correct, not just plausible-looking.

### Remaining work / honest limitations

See `sim/README.md` "Limitations" and "Future Enhancements" -- notably:
unweighted (not entitlement-normalized) Jain fairness, no CLI flags for
per-algorithm tuning, no `eevdf-lite` policy, the discordant
scheduling-delay finding is unresolved (by design -- see above), and no
CI pipeline has run against this code (not pushed to GitHub in this task).
