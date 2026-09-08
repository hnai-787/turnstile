#ifndef SCHEDSIM_METRICS_HPP
#define SCHEDSIM_METRICS_HPP

#include <map>
#include <vector>

#include "schedsim/types.hpp"

namespace schedsim {

struct TaskMetrics {
    int taskId;
    Time arrival = 0;
    Time completion = 0;
    Time turnaround = 0;        // completion - arrival
    Time response = -1;         // first dispatch - arrival (-1 if never dispatched, shouldn't happen)
    Time readyWait = 0;         // time spent runnable but not running (sum across all ready periods)
    Time cpuService = 0;        // total CPU time actually received
    Time ioBlocked = 0;         // total time spent blocked on I/O
    Time maxDispatchDelay = 0;  // worst single ready-to-dispatch gap (mirrors `perf sched latency`'s "max delay")
    std::uint64_t dispatchCount = 0;
    std::uint64_t preemptionCount = 0;
    double slowdown = 0.0;      // turnaround / cpuService ("stretch"); undefined (0) if cpuService == 0
};

struct SystemMetrics {
    Time simulationDuration = 0;
    Time busyTime = 0;
    double utilization = 0.0;  // busyTime / simulationDuration
    double throughput = 0.0;   // tasks completed / simulationDuration
    std::uint64_t contextSwitches = 0;
    std::uint64_t preemptions = 0;
    // Unweighted Jain's fairness index over per-task CPU service time.
    // Appropriate for equal-priority/equal-nice workloads; for a
    // deliberately unequal (nice-mix) workload this will correctly read
    // as "unfair" even when CfsLite is behaving exactly as designed --
    // see README "Design decisions" for why proportional-share workloads
    // are validated with a separate ratio comparison instead of folding
    // entitlement-weighting into this single number.
    double jainFairnessRaw = 0.0;
};

struct RunResult {
    std::string policyName;
    std::map<int, TaskMetrics> perTask;  // keyed by task id
    SystemMetrics system;
};

double jainIndex(const std::vector<double>& values);

}  // namespace schedsim

#endif
