#include <catch2/catch_test_macros.hpp>

#include "schedsim/runner.hpp"

using namespace schedsim;

namespace {
Task cpuTask(int id, Time arrival, Time cpuDemand) {
    Task t;
    t.id = id;
    t.arrival = arrival;
    t.bursts = {{BurstKind::Cpu, cpuDemand}};
    return t;
}
}  // namespace

TEST_CASE("a single task runs to completion immediately under RR", "[simulator]") {
    Workload w;
    w.tasks = {cpuTask(1, 0, 100)};
    RunResult r = runWorkload(w, AlgoSpec{"rr", 100});
    REQUIRE(r.perTask.at(1).completion == 100);
    REQUIRE(r.perTask.at(1).response == 0);
    REQUIRE(r.perTask.at(1).cpuService == 100);
}

TEST_CASE("two equal tasks alternate under RR with a quantum shorter than their demand", "[simulator]") {
    // Both arrive at 0, quantum=10, each needs 20: expect A,B,A,B interleaving -> both finish at 40, but B
    // (dispatched second each round) finishes marginally after A's final segment.
    Workload w;
    w.tasks = {cpuTask(1, 0, 20), cpuTask(2, 0, 20)};
    RunResult r = runWorkload(w, AlgoSpec{"rr", 10});
    // A: 0-10, B: 10-20, A: 20-30, B: 30-40
    REQUIRE(r.perTask.at(1).completion == 30);
    REQUIRE(r.perTask.at(2).completion == 40);
    REQUIRE(r.perTask.at(1).dispatchCount == 2);
    REQUIRE(r.perTask.at(2).dispatchCount == 2);
}

TEST_CASE("I/O round trip: a task blocks and resumes correctly", "[simulator]") {
    Workload w;
    Task t;
    t.id = 1;
    t.arrival = 0;
    t.bursts = {{BurstKind::Cpu, 5}, {BurstKind::Io, 20}, {BurstKind::Cpu, 5}};
    w.tasks = {t};
    RunResult r = runWorkload(w, AlgoSpec{"rr", 100});
    // 0-5 CPU, 5-25 IO, 25-30 CPU -> completion at 30
    REQUIRE(r.perTask.at(1).completion == 30);
    REQUIRE(r.perTask.at(1).cpuService == 10);
    REQUIRE(r.perTask.at(1).ioBlocked == 20);
}

TEST_CASE("a late arrival does not run before it arrives", "[simulator]") {
    Workload w;
    w.tasks = {cpuTask(1, 0, 10), cpuTask(2, 50, 10)};
    RunResult r = runWorkload(w, AlgoSpec{"rr", 100});
    REQUIRE(r.perTask.at(2).response >= 0);
    REQUIRE(r.perTask.at(2).completion >= 60);
}

TEST_CASE("priority: a high-priority arrival preempts a running low-priority task", "[simulator]") {
    Workload w;
    Task low = cpuTask(1, 0, 100);
    low.priority = 5;
    Task high = cpuTask(2, 10, 5);
    high.priority = 0;
    w.tasks = {low, high};
    RunResult r = runWorkload(w, AlgoSpec{"priority"});
    // High-priority task arrives at 10 while low is running; it should
    // preempt immediately and finish before the low-priority task resumes.
    REQUIRE(r.perTask.at(2).completion == 15);
    REQUIRE(r.system.preemptions >= 1);
}

TEST_CASE("mlfq: a CPU-bound task is demoted after exhausting its top-level allotment", "[simulator]") {
    MlfqConfig cfg;
    cfg.quanta = {10, 20};
    cfg.allotments = {20, 1000000};
    cfg.boostInterval = 0;
    Workload w;
    w.tasks = {cpuTask(1, 0, 100)};
    AlgoSpec spec;
    spec.name = "mlfq";
    spec.mlfqConfig = cfg;
    RunResult r = runWorkload(w, spec);
    REQUIRE(r.perTask.at(1).completion == 100);
    // At quantum=10 and top-level allotment=20, the task should be
    // dispatched twice at level 0 (10+10=20, exhausting the allotment)
    // then demoted to level 1 (quantum 20) for the remainder (80 more
    // units / 20 = 4 more dispatches) -> 6 dispatches total.
    REQUIRE(r.perTask.at(1).dispatchCount == 6);
}

TEST_CASE("cfs-lite: equal-nice tasks receive equal CPU service over a long run", "[simulator]") {
    Workload w;
    w.tasks = {cpuTask(1, 0, 1000), cpuTask(2, 0, 1000)};
    RunResult r = runWorkload(w, AlgoSpec{"cfs-lite", 100, {}, MlfqConfig::ostepDemo(), 5});
    REQUIRE(r.perTask.at(1).cpuService == 1000);
    REQUIRE(r.perTask.at(2).cpuService == 1000);
    // Both finish; total makespan should be close to 2000 (some overlap
    // scheduling overhead is fine, but neither task should be able to
    // finish drastically before the other given equal weights).
    REQUIRE(std::abs(static_cast<long long>(r.perTask.at(1).completion) -
                      static_cast<long long>(r.perTask.at(2).completion)) <= 5);
}
