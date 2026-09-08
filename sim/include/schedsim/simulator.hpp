#ifndef SCHEDSIM_SIMULATOR_HPP
#define SCHEDSIM_SIMULATOR_HPP

#include <map>
#include <memory>

#include "schedsim/metrics.hpp"
#include "schedsim/policy.hpp"
#include "schedsim/types.hpp"

namespace schedsim {

struct TaskRuntimeState {
    const Task* task = nullptr;
    std::size_t burstIndex = 0;
    Time progressInBurst = 0;
    Time lastReadyTime = 0;
    Time blockedSince = 0;
    bool firstDispatchDone = false;
    TaskMetrics metrics;
};

// A discrete-event simulator: it always jumps directly to the next
// relevant event (a burst/slice boundary, an arrival, or an I/O
// completion) rather than stepping tick by tick. See README "Design
// decisions".
class Simulator {
public:
    Simulator(Workload workload, std::unique_ptr<SchedulerPolicy> policy);
    RunResult run();

private:
    Workload workload_;
    std::unique_ptr<SchedulerPolicy> policy_;
    std::map<int, TaskRuntimeState> state_;
};

}  // namespace schedsim

#endif
