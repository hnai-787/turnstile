#ifndef SCHEDSIM_WORKLOAD_PRESETS_HPP
#define SCHEDSIM_WORKLOAD_PRESETS_HPP

#include <vector>

#include "schedsim/types.hpp"

namespace schedsim::presets {

// N equal tasks, all arriving at time 0, each a single long CPU burst,
// equal priority/nice. Tests baseline fair sharing under pure CPU load.
Workload cpuBoundEqual(int taskCount = 4, Time cpuDemand = 1000);

// Tasks with deliberately different CPU demands, all arriving at time 0.
// Tests completion order and slowdown/stretch effects.
Workload cpuBoundMixed();

// A mix of "interactive" tasks (short CPU bursts alternating with I/O)
// and one long CPU-bound hog, all arriving near time 0. The canonical
// scenario for showing MLFQ/priority responsiveness advantages over
// plain round-robin.
Workload interactiveMix();

// Same job shape (one long CPU burst each), different `nice` values.
// Tests CfsLite's proportional-share property directly.
Workload niceMix();

// A continuous stream of short high-priority tasks competing against one
// long low-priority task. Tests starvation and the aging/priority-boost
// anti-starvation mechanisms.
Workload starvation();

}  // namespace schedsim::presets

#endif
