# Historical reference: real Linux 6.12.25 kernel measurement

This directory is **immutable historical data**, not generated output. It
is the real, measured result of actually recompiling Linux 6.12.25 (Kali
Rolling 2025.2) with `kernel/sched/fair.c` patched to a round-robin
scheduler, booting it in a VM, and running the same benchmark suite
(sysbench CPU/fileio, stress-ng, `perf sched latency`, a `top` snapshot)
against both the patched kernel and the unmodified one. See
`archive/academic-original/` for the original patch source and benchmark
scripts that produced this data, and the workspace root README for the
full, honest interpretation.

**Do not regenerate, edit, or "clean" the files in `round-robin/` and
`vanilla-fair/`** — they are the actual experimental record. The
simulation framework (`sim/`) is validated *against* this data; this data
is never derived *from* the simulator.

## A factual correction from this project's rebuild

The original README described the unmodified baseline as "CFS" (the
Completely Fair Scheduler). That's the traditional name for Linux's fair
scheduling class, but it is not fully accurate for kernel 6.12: Linux
began transitioning the fair scheduling class from classic CFS to
**EEVDF** (Earliest Eligible Virtual Deadline First) in kernel 6.6.
`fair.c` in a 6.12 kernel still maintains weighted `vruntime`, but task
selection uses EEVDF eligibility and virtual deadlines (`pick_eevdf`), not
the classic "always run the leftmost node" CFS rule. See `environment.json`.

This does not change what was actually measured — the real benchmark data
here is unaffected — it changes how the *baseline scheduler* should be
named and described. The simulation framework's `cfs-lite` policy is a
pedagogical approximation of **classic** CFS (weighted vruntime, pick
minimum), explicitly *not* a claim to reproduce Linux 6.12's actual
EEVDF-era behavior. See `sim/README.md` "Design decisions".

## Files

- `round-robin/`, `vanilla-fair/` — the real benchmark output (copied,
  unmodified, from the original `results/kernel_roundrobin/` and
  `results/kernel_vanilla/`).
- `kernel_comparison_report.html` — the original auto-generated
  comparison report (its "winner" framing is a naive sum-of-metrics
  heuristic — see the workspace README for the more nuanced reading).
- `environment.json` — a provenance manifest: what was measured, on what,
  once, with what known limitations.
