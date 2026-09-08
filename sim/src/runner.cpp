#include "schedsim/runner.hpp"

#include <stdexcept>

#include "schedsim/simulator.hpp"

namespace schedsim {

RunResult runWorkload(const Workload& workload, const AlgoSpec& spec) {
    std::unique_ptr<SchedulerPolicy> policy;

    if (spec.name == "rr") {
        policy = std::make_unique<RoundRobinPolicy>(spec.rrQuantum);
    } else if (spec.name == "priority") {
        auto p = std::make_unique<PriorityPolicy>(spec.priorityConfig);
        for (const Task& t : workload.tasks) p->registerTask(t.id, t.priority);
        policy = std::move(p);
    } else if (spec.name == "mlfq") {
        policy = std::make_unique<MlfqPolicy>(spec.mlfqConfig);
    } else if (spec.name == "cfs-lite") {
        auto p = std::make_unique<CfsLitePolicy>(spec.cfsGranularity);
        for (const Task& t : workload.tasks) p->registerTask(t.id, t.nice);
        policy = std::move(p);
    } else {
        throw std::invalid_argument("runWorkload: unknown algorithm \"" + spec.name + "\"");
    }

    Simulator sim(workload, std::move(policy));
    return sim.run();
}

}  // namespace schedsim
