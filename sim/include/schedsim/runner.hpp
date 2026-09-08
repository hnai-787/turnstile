#ifndef SCHEDSIM_RUNNER_HPP
#define SCHEDSIM_RUNNER_HPP

#include <string>

#include "schedsim/metrics.hpp"
#include "schedsim/policies.hpp"
#include "schedsim/types.hpp"

namespace schedsim {

struct AlgoSpec {
    std::string name;  // "rr" | "priority" | "mlfq" | "cfs-lite"
    Time rrQuantum = 100;
    PriorityConfig priorityConfig{};
    MlfqConfig mlfqConfig = MlfqConfig::ostepDemo();
    Time cfsGranularity = 4;
};

RunResult runWorkload(const Workload& workload, const AlgoSpec& spec);

}  // namespace schedsim

#endif
