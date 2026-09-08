#include "schedsim/simulator.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>

namespace schedsim {

namespace {
constexpr Time kInfinity = std::numeric_limits<Time>::max();
}

Time Task::totalCpuDemand() const {
    Time sum = 0;
    for (const Burst& b : bursts) {
        if (b.kind == BurstKind::Cpu) sum += b.duration;
    }
    return sum;
}

Simulator::Simulator(Workload workload, std::unique_ptr<SchedulerPolicy> policy)
    : workload_(std::move(workload)), policy_(std::move(policy)) {
    // workload_ is already in its final storage location at this point
    // (assigned via the member initializer list above), so it's safe to
    // take stable pointers into workload_.tasks here.
    for (const Task& t : workload_.tasks) {
        if (t.bursts.empty()) throw std::invalid_argument("Simulator: task " + std::to_string(t.id) + " has no bursts");
        TaskRuntimeState st;
        st.task = &t;
        state_[t.id] = st;
    }
}

RunResult Simulator::run() {
    std::vector<const Task*> arrivalsSorted;
    for (const Task& t : workload_.tasks) arrivalsSorted.push_back(&t);
    std::sort(arrivalsSorted.begin(), arrivalsSorted.end(),
              [](const Task* a, const Task* b) { return a->arrival != b->arrival ? a->arrival < b->arrival : a->id < b->id; });
    std::size_t nextArrivalIdx = 0;

    std::multimap<Time, int> blocked;  // completion time -> task id

    std::optional<int> currentTaskId;
    Time currentSegmentStart = 0;
    Time dispatchedAt = 0;  // when currentTaskId was most recently freshly dispatched by the policy
    std::optional<Time> currentDispatchMaxEnd;

    Time time = 0;
    Time busyTime = 0;
    std::uint64_t contextSwitches = 0;
    std::uint64_t preemptions = 0;
    std::size_t completed = 0;
    const std::size_t total = workload_.tasks.size();

    auto accountUpTo = [&](Time now) {
        if (!currentTaskId) return;
        Time elapsed = now - currentSegmentStart;
        if (elapsed > 0) {
            TaskRuntimeState& st = state_[*currentTaskId];
            st.progressInBurst += elapsed;
            st.metrics.cpuService += elapsed;
            busyTime += elapsed;
        }
        currentSegmentStart = now;
    };

    auto requeueAfterPreempt = [&](int taskId, Time now, Time elapsed) {
        state_[taskId].lastReadyTime = now;
        policy_->onPreempted(taskId, now, elapsed);
    };

    auto tryPreemptForNewReady = [&](Time now) {
        if (currentTaskId && policy_->shouldPreemptCurrent(*currentTaskId, now)) {
            accountUpTo(now);
            int id = *currentTaskId;
            Time elapsed = now - dispatchedAt;
            currentTaskId.reset();
            currentDispatchMaxEnd.reset();
            ++preemptions;
            requeueAfterPreempt(id, now, elapsed);
        }
    };

    auto bringInArrivals = [&](Time now) {
        while (nextArrivalIdx < arrivalsSorted.size() && arrivalsSorted[nextArrivalIdx]->arrival <= now) {
            const Task* task = arrivalsSorted[nextArrivalIdx++];
            TaskRuntimeState& st = state_[task->id];
            st.lastReadyTime = task->arrival;
            st.metrics.taskId = task->id;
            st.metrics.arrival = task->arrival;
            policy_->onArrive(task->id, task->arrival);
            tryPreemptForNewReady(now);
        }
    };

    auto bringInIo = [&](Time now) {
        while (!blocked.empty() && blocked.begin()->first <= now) {
            auto it = blocked.begin();
            Time completionTime = it->first;
            int taskId = it->second;
            blocked.erase(it);
            TaskRuntimeState& st = state_[taskId];
            st.metrics.ioBlocked += completionTime - st.blockedSince;
            ++st.burstIndex;
            st.progressInBurst = 0;
            if (st.burstIndex >= st.task->bursts.size()) {
                // The task's last burst was an I/O burst -- it finishes
                // here, not by returning to the ready queue. Without this
                // check, dispatch() would later index st.task->bursts
                // out of bounds (this was a real bug caught by this
                // project's own test suite -- see PROJECT_NOTES.md).
                st.metrics.completion = completionTime;
                st.metrics.turnaround = completionTime - st.metrics.arrival;
                st.metrics.slowdown = st.metrics.cpuService > 0
                                           ? static_cast<double>(st.metrics.turnaround) / static_cast<double>(st.metrics.cpuService)
                                           : 0.0;
                policy_->onComplete(taskId, completionTime, 0);
                ++completed;
            } else {
                st.lastReadyTime = completionTime;
                policy_->onReady(taskId, completionTime);
                tryPreemptForNewReady(completionTime);
            }
        }
    };

    while (completed < total) {
        bringInArrivals(time);
        bringInIo(time);

        if (!currentTaskId) {
            DispatchDecision d = policy_->dispatch(time);
            if (!d.taskId) {
                Time next = kInfinity;
                if (nextArrivalIdx < arrivalsSorted.size()) next = std::min(next, arrivalsSorted[nextArrivalIdx]->arrival);
                if (!blocked.empty()) next = std::min(next, blocked.begin()->first);
                if (next == kInfinity) break;  // nothing left to happen; guards against a stuck workload
                time = next;
                continue;
            }
            TaskRuntimeState& st = state_[*d.taskId];
            Time delay = time - st.lastReadyTime;
            st.metrics.readyWait += delay;
            st.metrics.maxDispatchDelay = std::max(st.metrics.maxDispatchDelay, delay);
            if (!st.firstDispatchDone) {
                st.metrics.response = time - st.metrics.arrival;
                st.firstDispatchDone = true;
            }
            ++st.metrics.dispatchCount;
            ++contextSwitches;
            currentTaskId = d.taskId;
            currentSegmentStart = time;
            dispatchedAt = time;
            currentDispatchMaxEnd = d.maxRuntime ? std::optional<Time>(time + *d.maxRuntime) : std::nullopt;
        }

        TaskRuntimeState& st = state_[*currentTaskId];
        const Burst& burst = st.task->bursts[st.burstIndex];
        Time naturalEnd = currentSegmentStart + (burst.duration - st.progressInBurst);
        Time segmentEnd = currentDispatchMaxEnd ? std::min(naturalEnd, *currentDispatchMaxEnd) : naturalEnd;

        Time nextExternal = kInfinity;
        if (nextArrivalIdx < arrivalsSorted.size()) nextExternal = std::min(nextExternal, arrivalsSorted[nextArrivalIdx]->arrival);
        if (!blocked.empty()) nextExternal = std::min(nextExternal, blocked.begin()->first);

        Time stepEnd = std::min(segmentEnd, nextExternal);
        time = stepEnd;
        accountUpTo(time);

        if (time == segmentEnd) {
            int taskId = *currentTaskId;
            bool burstDone = st.progressInBurst >= burst.duration;
            Time elapsed = time - dispatchedAt;
            currentTaskId.reset();
            currentDispatchMaxEnd.reset();

            if (burstDone) {
                TaskRuntimeState& tst = state_[taskId];
                ++tst.burstIndex;
                tst.progressInBurst = 0;
                if (tst.burstIndex >= tst.task->bursts.size()) {
                    tst.metrics.completion = time;
                    tst.metrics.turnaround = time - tst.metrics.arrival;
                    tst.metrics.slowdown = tst.metrics.cpuService > 0
                                               ? static_cast<double>(tst.metrics.turnaround) / static_cast<double>(tst.metrics.cpuService)
                                               : 0.0;
                    policy_->onComplete(taskId, time, elapsed);
                    ++completed;
                } else if (tst.task->bursts[tst.burstIndex].kind == BurstKind::Io) {
                    tst.blockedSince = time;
                    Time completionTime = time + tst.task->bursts[tst.burstIndex].duration;
                    blocked.insert({completionTime, taskId});
                    policy_->onBlock(taskId, time, elapsed);
                } else {
                    // Consecutive CPU bursts: treat the boundary like a
                    // preemption/re-ready event so the policy re-queues it
                    // through its normal path rather than needing a
                    // separate hook for this uncommon case.
                    requeueAfterPreempt(taskId, time, elapsed);
                }
            } else {
                ++preemptions;
                state_[taskId].metrics.preemptionCount++;
                requeueAfterPreempt(taskId, time, elapsed);
            }
        }
    }

    RunResult result;
    result.policyName = policy_->name();
    for (auto& [id, st] : state_) result.perTask[id] = st.metrics;

    result.system.simulationDuration = time;
    result.system.busyTime = busyTime;
    result.system.utilization = time > 0 ? static_cast<double>(busyTime) / static_cast<double>(time) : 0.0;
    result.system.throughput = time > 0 ? static_cast<double>(completed) / static_cast<double>(time) : 0.0;
    result.system.contextSwitches = contextSwitches;
    result.system.preemptions = preemptions;

    std::vector<double> serviceTimes;
    for (auto& [id, m] : result.perTask) serviceTimes.push_back(static_cast<double>(m.cpuService));
    result.system.jainFairnessRaw = jainIndex(serviceTimes);

    return result;
}

}  // namespace schedsim
