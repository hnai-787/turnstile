#include "schedsim/workload_presets.hpp"

namespace schedsim::presets {

Workload cpuBoundEqual(int taskCount, Time cpuDemand) {
    Workload w;
    w.name = "cpu-bound-equal";
    for (int i = 1; i <= taskCount; ++i) {
        Task t;
        t.id = i;
        t.name = "cpu" + std::to_string(i);
        t.arrival = 0;
        t.bursts = {{BurstKind::Cpu, cpuDemand}};
        w.tasks.push_back(t);
    }
    return w;
}

Workload cpuBoundMixed() {
    Workload w;
    w.name = "cpu-bound-mixed";
    std::vector<Time> demands = {50, 100, 200, 400};
    int id = 1;
    for (Time d : demands) {
        Task t;
        t.id = id;
        t.name = "job" + std::to_string(id);
        t.arrival = 0;
        t.bursts = {{BurstKind::Cpu, d}};
        w.tasks.push_back(t);
        ++id;
    }
    return w;
}

Workload interactiveMix() {
    Workload w;
    w.name = "interactive-mix";

    // Three interactive tasks: short CPU bursts, long I/O waits, repeated.
    for (int i = 1; i <= 3; ++i) {
        Task t;
        t.id = i;
        t.name = "interactive" + std::to_string(i);
        t.arrival = i;  // slightly staggered
        for (int rep = 0; rep < 5; ++rep) {
            t.bursts.push_back({BurstKind::Cpu, 5});
            t.bursts.push_back({BurstKind::Io, 40});
        }
        t.bursts.push_back({BurstKind::Cpu, 5});
        w.tasks.push_back(t);
    }

    // One CPU-bound hog competing for the same CPU.
    Task hog;
    hog.id = 100;
    hog.name = "cpu-hog";
    hog.arrival = 0;
    hog.bursts = {{BurstKind::Cpu, 2000}};
    w.tasks.push_back(hog);

    return w;
}

Workload niceMix() {
    Workload w;
    w.name = "nice-mix";
    struct Entry { int id; int nice; };
    std::vector<Entry> entries = {{1, -10}, {2, 0}, {3, 10}};
    for (const Entry& e : entries) {
        Task t;
        t.id = e.id;
        t.name = "nice" + std::to_string(e.nice);
        t.arrival = 0;
        t.nice = e.nice;
        t.bursts = {{BurstKind::Cpu, 3000}};
        w.tasks.push_back(t);
    }
    return w;
}

Workload starvation() {
    Workload w;
    w.name = "starvation";

    // One long, low-priority background task.
    Task background;
    background.id = 1;
    background.name = "background";
    background.arrival = 0;
    background.priority = 5;  // low priority (higher number)
    background.bursts = {{BurstKind::Cpu, 5000}};
    w.tasks.push_back(background);

    // A steady stream of short, high-priority tasks arriving over time.
    for (int i = 0; i < 20; ++i) {
        Task t;
        t.id = 100 + i;
        t.name = "urgent" + std::to_string(i);
        t.arrival = i * 30;
        t.priority = 0;  // high priority
        t.bursts = {{BurstKind::Cpu, 10}};
        w.tasks.push_back(t);
    }

    return w;
}

}  // namespace schedsim::presets
