#ifndef SCHEDSIM_POLICY_HPP
#define SCHEDSIM_POLICY_HPP

#include <optional>

#include "schedsim/types.hpp"

namespace schedsim {

struct DispatchDecision {
    std::optional<int> taskId;
    // nullopt = run until the current CPU burst completes naturally or an
    // external event (arrival/IO completion) forces a re-dispatch check.
    // A bounded value models a quantum (RR), an MLFQ level allotment
    // remainder, or CFS-lite's periodic re-evaluation granularity.
    std::optional<Time> maxRuntime;
};

// The simulator owns time and the CPU; a policy only owns the decision of
// *which ready task to run next* and *how the ready set is organized*.
// See README "Design decisions" for why this is a discrete-event design
// (the simulator jumps to the next relevant event) rather than a
// millisecond-by-millisecond tick loop.
class SchedulerPolicy {
public:
    virtual ~SchedulerPolicy() = default;

    virtual std::string name() const = 0;

    // A task becomes ready for the very first time.
    virtual void onArrive(int taskId, Time now) = 0;
    // A task becomes ready again after finishing an I/O burst.
    virtual void onReady(int taskId, Time now) = 0;
    // The currently running task blocked on an I/O burst (already removed
    // from "running" by the simulator). `elapsed` is how much CPU time it
    // actually consumed since it was last dispatched -- needed by e.g.
    // MLFQ's allotment accounting (OSTEP Rule 4: allotment is consumed
    // across yields, not reset by them).
    virtual void onBlock(int taskId, Time now, Time elapsed) = 0;
    // The currently running task finished its last burst entirely.
    virtual void onComplete(int taskId, Time now, Time elapsed) = 0;
    // The currently running task's dispatch ended (quantum/allotment/
    // granularity expired, or it was preempted by a higher-priority
    // arrival) but it still has work left; the policy must decide where
    // it goes in its own ready structure.
    virtual void onPreempted(int taskId, Time now, Time elapsed) = 0;

    // Called whenever the CPU is idle and at least one task is ready.
    virtual DispatchDecision dispatch(Time now) = 0;

    // Called whenever a task becomes newly ready (arrival or IO
    // completion) while the CPU is busy, so preemptive policies can
    // interrupt the current runner. Non-preemptive policies (RR,
    // CFS-lite in this implementation) always return false and instead
    // rely on `dispatch`'s bounded `maxRuntime` to periodically
    // re-evaluate who should run.
    virtual bool shouldPreemptCurrent(int currentTaskId, Time now) const = 0;
};

}  // namespace schedsim

#endif
