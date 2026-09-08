#ifndef SCHEDSIM_TYPES_HPP
#define SCHEDSIM_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace schedsim {

// Abstract simulation time unit -- deliberately not called "ms" anywhere
// in the core model. Workload authors can treat it as microseconds,
// milliseconds, or anything else; what matters for validation against the
// real kernel measurement is *relative* behavior (rankings, directions of
// effect), not unit-for-unit equivalence. See README "Design decisions".
using Time = std::int64_t;

enum class BurstKind { Cpu, Io };

struct Burst {
    BurstKind kind;
    Time duration;
};

struct Task {
    // Every field defaults to a safe, deterministic value: a Task built
    // field-by-field (as opposed to via parseWorkloadJson, which always
    // sets every field explicitly) that forgets to set `id` or `arrival`
    // must not silently pick up indeterminate stack memory -- exactly
    // the bug this project's own test suite caught in an early draft of
    // a hand-built test workload (see PROJECT_NOTES.md).
    int id = 0;
    std::string name;
    Time arrival = 0;
    int priority = 0;  // lower value = higher priority (used by the Priority and MLFQ policies)
    int nice = 0;       // -20..19, Linux convention (used by the CfsLite policy only)
    std::vector<Burst> bursts;

    Time totalCpuDemand() const;
};

struct Workload {
    std::string name;
    std::vector<Task> tasks;
};

}  // namespace schedsim

#endif
