#ifndef SCHEDSIM_POLICIES_HPP
#define SCHEDSIM_POLICIES_HPP

#include <deque>
#include <map>
#include <memory>
#include <vector>

#include "schedsim/policy.hpp"

namespace schedsim {

// Classic round-robin: one FIFO ready queue, a fixed quantum, no
// preemption on arrival (new arrivals just join the tail).
class RoundRobinPolicy : public SchedulerPolicy {
public:
    explicit RoundRobinPolicy(Time quantum);
    std::string name() const override { return "rr"; }
    void onArrive(int taskId, Time now) override;
    void onReady(int taskId, Time now) override;
    void onBlock(int taskId, Time now, Time elapsed) override;
    void onComplete(int taskId, Time now, Time elapsed) override;
    void onPreempted(int taskId, Time now, Time elapsed) override;
    DispatchDecision dispatch(Time now) override;
    bool shouldPreemptCurrent(int currentTaskId, Time now) const override;

private:
    Time quantum_;
    std::deque<int> queue_;
};

struct PriorityConfig {
    Time quantum = 50;         // round-robin slice among equal-priority tasks (OSTEP rule 2)
    Time agingInterval = 0;    // 0 disables aging; otherwise, boost a task waiting this long
    int agingBoost = 1;
};

// Fixed-priority preemptive scheduling: lower `priority` value = runs
// first; equal-priority tasks round-robin against each other; an
// optional aging mechanism prevents starvation by promoting long-waiting
// tasks.
class PriorityPolicy : public SchedulerPolicy {
public:
    explicit PriorityPolicy(PriorityConfig config);
    std::string name() const override { return "priority"; }
    void onArrive(int taskId, Time now) override;
    void onReady(int taskId, Time now) override;
    void onBlock(int taskId, Time now, Time elapsed) override;
    void onComplete(int taskId, Time now, Time elapsed) override;
    void onPreempted(int taskId, Time now, Time elapsed) override;
    DispatchDecision dispatch(Time now) override;
    bool shouldPreemptCurrent(int currentTaskId, Time now) const override;

    // Registers a task's static priority; must be called once per task
    // before the simulation starts (the policy has no other way to learn
    // priorities, since the SchedulerPolicy interface only passes ids).
    void registerTask(int taskId, int priority);

private:
    void applyAging(Time now);
    void enqueue(int taskId, Time now);

    PriorityConfig config_;
    std::map<int, int> basePriority_;
    std::map<int, int> effectivePriority_;
    std::map<int, Time> waitingSince_;
    std::map<int, std::deque<int>> readyQueues_;  // priority -> FIFO queue
};

struct MlfqConfig {
    std::vector<Time> quanta;      // per level, index 0 = highest priority
    std::vector<Time> allotments;  // per level: total CPU time before demotion (OSTEP rule 4)
    Time boostInterval = 0;        // 0 disables periodic priority boost (OSTEP rule 5)

    // A named, citable starting point (not "the" MLFQ standard -- OSTEP
    // is explicit that these constants are workload-dependent tuning
    // parameters, not a specification). See README "Design decisions".
    static MlfqConfig ostepDemo();
};

// Multi-level feedback queue following OSTEP's five rules, including the
// anti-gaming Rule 4 (allotment is consumed cumulatively across a level,
// regardless of how many times the task yielded for I/O in between).
class MlfqPolicy : public SchedulerPolicy {
public:
    explicit MlfqPolicy(MlfqConfig config);
    std::string name() const override { return "mlfq"; }
    void onArrive(int taskId, Time now) override;
    void onReady(int taskId, Time now) override;
    void onBlock(int taskId, Time now, Time elapsed) override;
    void onComplete(int taskId, Time now, Time elapsed) override;
    void onPreempted(int taskId, Time now, Time elapsed) override;
    DispatchDecision dispatch(Time now) override;
    bool shouldPreemptCurrent(int currentTaskId, Time now) const override;

private:
    void maybeBoost(Time now);
    void consumeAllotment(int taskId, Time elapsed);

    MlfqConfig config_;
    std::vector<std::deque<int>> queues_;
    std::map<int, std::size_t> level_;
    std::map<int, Time> allotmentUsed_;
    Time lastBoost_ = 0;
};

// A pedagogical approximation of *classic* CFS (weighted virtual
// runtime, pick minimum) using Linux's real nice-to-weight table. This is
// explicitly not a claim to model Linux 6.12's actual EEVDF-era fair
// scheduling class -- see README "Design decisions" and
// validation/kernel-6.12.25/README.md.
class CfsLitePolicy : public SchedulerPolicy {
public:
    // `granularity` bounds each dispatch so the policy periodically
    // re-evaluates which task has the minimum vruntime, standing in for
    // real CFS's continuous re-scheduling without requiring true
    // resumable microcycle stepping (see README "Design decisions").
    explicit CfsLitePolicy(Time granularity);
    std::string name() const override { return "cfs-lite"; }
    void onArrive(int taskId, Time now) override;
    void onReady(int taskId, Time now) override;
    void onBlock(int taskId, Time now, Time elapsed) override;
    void onComplete(int taskId, Time now, Time elapsed) override;
    void onPreempted(int taskId, Time now, Time elapsed) override;
    DispatchDecision dispatch(Time now) override;
    bool shouldPreemptCurrent(int currentTaskId, Time now) const override;

    void registerTask(int taskId, int nice);

    static int weightForNice(int nice);

private:
    Time granularity_;
    std::map<int, int> weight_;
    std::map<int, double> vruntime_;
    std::vector<int> ready_;  // unsorted; dispatch() does a linear min-scan (see README, "no RB-tree needed")
    double minVruntimeSeen_ = 0.0;  // approximates real CFS's place_entity: a joining task is never
                                     // placed behind the current pack, so a stale low vruntime (a
                                     // fresh arrival's default 0, or a task that's been blocked on
                                     // I/O for a long time) can't let it monopolize the CPU
};

}  // namespace schedsim

#endif
