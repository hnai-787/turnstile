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
