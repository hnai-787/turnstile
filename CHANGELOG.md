# Changelog

All notable changes to this project are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

### Changed

### Fixed

## [1.0.0] - 2026-09-08

### Added

- **schedsim** (`sim/`): a new discrete-event C++ scheduling simulation
  framework implementing round-robin, fixed-priority (with optional
  aging), OSTEP-style MLFQ (including the exact cumulative-allotment
  anti-gaming Rule 4), and a classic-CFS approximation (`cfs-lite`, using
  Linux's real 40-entry nice-to-weight table).
- Five workload presets (`cpu-bound-equal`, `cpu-bound-mixed`,
  `interactive-mix`, `nice-mix`, `starvation`) plus a JSON workload format
  for custom scenarios.
- Per-task and system-level metrics: turnaround, response, ready-wait,
  scheduling delay (mirroring `perf sched latency`), slowdown, context
  switches, preemptions, and Jain's fairness index.
- `schedsim validate`: compares simulated scheduler-level trends against
  the real historical kernel measurement using an explicit three-tier
  methodology (direct / qualitative-direction-only / not-comparable),
  never comparing absolute values across the two very different
  measurement systems.
- `validation/kernel-6.12.25/`: the real kernel benchmark data, preserved
  as immutable historical reference (not generated output), with a
  provenance manifest (`environment.json`) documenting exactly what was
  measured and its limitations.
- 24 Catch2 test cases / 133 assertions covering every policy's specific
  behavior, end-to-end simulation correctness, and cross-cutting
  invariants (CPU conservation, exact demand satisfaction, RR
  non-starvation, CfsLite weighted proportional share) across every
  policy and workload preset.

### Changed

- Corrected a factual description carried over from the original report:
  a Linux 6.12 kernel's fair scheduling class is EEVDF-era (since 6.6),
  not classic CFS. The real measured data is unaffected; only the
  baseline's name/description needed correcting. See
  `validation/kernel-6.12.25/README.md`.
- Root `README.md` rewritten to document both halves of the project (the
  original kernel patch and the new simulation framework) and to report
  the historical-validation findings honestly, including one discordant result.

### Fixed

- The discrete-event simulator's I/O-completion handler never checked
  whether a just-finished I/O burst was a task's *last* burst, causing an
  out-of-bounds burst-array access (undefined behavior, surfaced as a
  corrupted `completion` timestamp) for any workload ending in I/O.
  Caught by this project's own test suite. Fixed by checking burst-array
  exhaustion in both places a task's last burst can end (CPU or I/O).
- `Task::id` and `Task::arrival` had no default member initializers, so a
  hand-built `Task` that forgot to set `.arrival` silently read
  indeterminate stack memory -- exactly what one of this project's own
  test workloads did. Fixed by giving every `Task` field a safe default.
