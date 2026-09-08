#include <catch2/catch_test_macros.hpp>

#include "schedsim/policies.hpp"

using namespace schedsim;

TEST_CASE("RoundRobinPolicy dispatches in FIFO arrival order with the configured quantum", "[policies]") {
    RoundRobinPolicy rr(25);
    rr.onArrive(1, 0);
    rr.onArrive(2, 0);
    DispatchDecision d1 = rr.dispatch(0);
    REQUIRE(d1.taskId == 1);
    REQUIRE(d1.maxRuntime == 25);
    DispatchDecision d2 = rr.dispatch(0);
    REQUIRE(d2.taskId == 2);
}

TEST_CASE("RoundRobinPolicy never preempts on new arrivals", "[policies]") {
    RoundRobinPolicy rr(25);
    rr.onArrive(1, 0);
    REQUIRE_FALSE(rr.shouldPreemptCurrent(1, 100));
}

TEST_CASE("PriorityPolicy dispatches the highest-priority (lowest number) ready task first", "[policies]") {
    PriorityPolicy p(PriorityConfig{});
    p.registerTask(1, 5);
    p.registerTask(2, 0);
    p.onArrive(1, 0);
    p.onArrive(2, 0);
    DispatchDecision d = p.dispatch(0);
    REQUIRE(d.taskId == 2);
}

TEST_CASE("PriorityPolicy aging promotes a long-waiting task into a strictly higher, otherwise-empty tier",
          "[policies]") {
    PriorityConfig cfg;
    cfg.agingInterval = 50;
    cfg.agingBoost = 1;
    PriorityPolicy p(cfg);
    p.registerTask(1, 5);  // arrives early, will be starved long enough to age
    p.registerTask(2, 5);  // same base priority, arrives late -- has NOT waited long enough to age yet
    p.onArrive(1, 0);
    p.onArrive(2, 60);
    // At t=60: task1 has waited 60 >= 50 (ages to priority 4); task2 has
    // waited 0 (no aging yet). Task1's boosted priority must win.
    DispatchDecision d = p.dispatch(60);
    REQUIRE(d.taskId == 1);
}

TEST_CASE("MlfqPolicy demotes only after the level's allotment is exhausted, cumulatively across yields",
          "[policies]") {
    // This directly tests OSTEP Rule 4 (the anti-gaming rule): a task
    // that yields (here, is preempted) multiple times before its
    // allotment is exhausted must still eventually be demoted based on
    // *cumulative* CPU consumption, not be reset to a fresh allotment by
    // each individual yield.
    MlfqConfig cfg;
    cfg.quanta = {10, 20};
    cfg.allotments = {25, 1000};
    cfg.boostInterval = 0;
    MlfqPolicy mlfq(cfg);
    mlfq.onArrive(1, 0);

    DispatchDecision d1 = mlfq.dispatch(0);
    REQUIRE(d1.taskId == 1);
    mlfq.onPreempted(1, 10, 10);  // consumed 10 of 25 allotment; still level 0

    DispatchDecision d2 = mlfq.dispatch(10);
    REQUIRE(d2.taskId == 1);
    mlfq.onBlock(1, 20, 10);  // consumed another 10 (I/O yield) -- 20/25 used, still level 0
    mlfq.onReady(1, 30);

    DispatchDecision d3 = mlfq.dispatch(30);
    REQUIRE(d3.taskId == 1);
    // Only 5 of the level's 10-unit quantum remain in the allotment.
    REQUIRE(d3.maxRuntime == 5);
}

TEST_CASE("MlfqPolicy periodic boost resets every known task to level 0", "[policies]") {
    MlfqConfig cfg;
    cfg.quanta = {10, 20, 40};
    cfg.allotments = {10, 10, 1000};
    cfg.boostInterval = 50;
    MlfqPolicy mlfq(cfg);
    mlfq.onArrive(1, 0);
    mlfq.dispatch(0);
    mlfq.onPreempted(1, 10, 10);  // exhausts level-0 allotment (10 >= 10) -> demoted to level 1

    DispatchDecision beforeBoost = mlfq.dispatch(10);
    REQUIRE(beforeBoost.taskId == 1);
    REQUIRE(beforeBoost.maxRuntime == 10);  // level 1's remaining allotment (10) caps the level-1 quantum (20)
    mlfq.onPreempted(1, 20, 10);

    DispatchDecision afterBoost = mlfq.dispatch(50);  // boostInterval elapsed -> back to level 0
    REQUIRE(afterBoost.taskId == 1);
    REQUIRE(afterBoost.maxRuntime == 10);  // level 0's quantum again
}

TEST_CASE("CfsLite weight table matches Linux's real nice-to-weight values at key anchors", "[policies]") {
    REQUIRE(CfsLitePolicy::weightForNice(0) == 1024);
    REQUIRE(CfsLitePolicy::weightForNice(-20) == 88761);
    REQUIRE(CfsLitePolicy::weightForNice(19) == 15);
    REQUIRE(CfsLitePolicy::weightForNice(5) == 335);
    REQUIRE(CfsLitePolicy::weightForNice(-5) == 3121);
}

TEST_CASE("CfsLite dispatches the ready task with the smallest vruntime", "[policies]") {
    CfsLitePolicy cfs(10);
    cfs.registerTask(1, 0);
    cfs.registerTask(2, 0);
    cfs.onArrive(1, 0);
    cfs.onArrive(2, 0);
    DispatchDecision d1 = cfs.dispatch(0);
    REQUIRE(d1.taskId == 1);  // tie broken by insertion order; both start at vruntime 0
    cfs.onPreempted(1, 10, 10);  // task 1's vruntime now advances past 0
    DispatchDecision d2 = cfs.dispatch(10);
    REQUIRE(d2.taskId == 2);  // task 2 still at vruntime 0 -- must run next
}
