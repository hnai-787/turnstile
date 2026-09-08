#include "schedsim/policies.hpp"

#include <algorithm>

namespace schedsim {

// ---------------------------------------------------------------- RoundRobin

RoundRobinPolicy::RoundRobinPolicy(Time quantum) : quantum_(quantum) {}

void RoundRobinPolicy::onArrive(int taskId, Time) { queue_.push_back(taskId); }
void RoundRobinPolicy::onReady(int taskId, Time) { queue_.push_back(taskId); }
void RoundRobinPolicy::onBlock(int, Time, Time) {}
void RoundRobinPolicy::onComplete(int, Time, Time) {}
void RoundRobinPolicy::onPreempted(int taskId, Time, Time) { queue_.push_back(taskId); }

DispatchDecision RoundRobinPolicy::dispatch(Time) {
    if (queue_.empty()) return {};
    int id = queue_.front();
    queue_.pop_front();
    return {id, quantum_};
}

bool RoundRobinPolicy::shouldPreemptCurrent(int, Time) const { return false; }

// ----------------------------------------------------------------- Priority

PriorityPolicy::PriorityPolicy(PriorityConfig config) : config_(config) {}

void PriorityPolicy::registerTask(int taskId, int priority) { basePriority_[taskId] = priority; }

void PriorityPolicy::enqueue(int taskId, Time now) {
    readyQueues_[effectivePriority_[taskId]].push_back(taskId);
    waitingSince_[taskId] = now;
}

void PriorityPolicy::onArrive(int taskId, Time now) {
    effectivePriority_[taskId] = basePriority_[taskId];
    enqueue(taskId, now);
}

void PriorityPolicy::onReady(int taskId, Time now) {
    effectivePriority_[taskId] = basePriority_[taskId];  // resumes at its base priority after I/O
    enqueue(taskId, now);
}

void PriorityPolicy::onBlock(int, Time, Time) {}
void PriorityPolicy::onComplete(int, Time, Time) {}

void PriorityPolicy::onPreempted(int taskId, Time now, Time) {
    enqueue(taskId, now);  // keeps whatever effective (possibly aged) priority it had
}

void PriorityPolicy::applyAging(Time now) {
    if (config_.agingInterval <= 0) return;
    for (auto& [prio, q] : readyQueues_) {
        std::deque<int> remaining;
        for (int id : q) {
            if (prio > 0 && now - waitingSince_[id] >= config_.agingInterval) {
                int newPrio = std::max(0, prio - config_.agingBoost);
                effectivePriority_[id] = newPrio;
                waitingSince_[id] = now;
                readyQueues_[newPrio].push_back(id);  // safe: std::map insertion doesn't invalidate other iterators
            } else {
                remaining.push_back(id);
            }
        }
        q = std::move(remaining);
    }
}

DispatchDecision PriorityPolicy::dispatch(Time now) {
    applyAging(now);
    for (auto& [prio, q] : readyQueues_) {
        if (!q.empty()) {
            int id = q.front();
            q.pop_front();
            return {id, config_.quantum};
        }
    }
    return {};
}

bool PriorityPolicy::shouldPreemptCurrent(int currentTaskId, Time) const {
    int currentPrio = effectivePriority_.at(currentTaskId);
    for (const auto& [prio, q] : readyQueues_) {
        if (prio >= currentPrio) break;  // map iterates in ascending key order
        if (!q.empty()) return true;
    }
    return false;
}

// -------------------------------------------------------------------- MLFQ

MlfqConfig MlfqConfig::ostepDemo() {
    // Values illustrating OSTEP's worked example shape (shorter quanta at
    // higher priority, progressively longer lower down) -- a named
    // teaching preset, not a claimed standard. See README "Design decisions".
    MlfqConfig c;
    c.quanta = {10, 20, 40};
    c.allotments = {20, 40, 80};
    c.boostInterval = 300;
    return c;
}

MlfqPolicy::MlfqPolicy(MlfqConfig config) : config_(std::move(config)) { queues_.resize(config_.quanta.size()); }

void MlfqPolicy::onArrive(int taskId, Time) {
    level_[taskId] = 0;
    allotmentUsed_[taskId] = 0;
    queues_[0].push_back(taskId);
}

void MlfqPolicy::onReady(int taskId, Time now) {
    queues_[level_[taskId]].push_back(taskId);  // I/O doesn't change level, only exhausting the allotment does
    maybeBoost(now);
}

void MlfqPolicy::consumeAllotment(int taskId, Time elapsed) {
    allotmentUsed_[taskId] += elapsed;
    std::size_t lvl = level_[taskId];
    if (lvl + 1 < queues_.size() && allotmentUsed_[taskId] >= config_.allotments[lvl]) {
        level_[taskId] = lvl + 1;
        allotmentUsed_[taskId] = 0;
    }
}

void MlfqPolicy::onBlock(int taskId, Time, Time elapsed) { consumeAllotment(taskId, elapsed); }

void MlfqPolicy::onComplete(int taskId, Time, Time elapsed) {
    consumeAllotment(taskId, elapsed);
    level_.erase(taskId);
    allotmentUsed_.erase(taskId);
}

void MlfqPolicy::onPreempted(int taskId, Time now, Time elapsed) {
    consumeAllotment(taskId, elapsed);
    queues_[level_[taskId]].push_back(taskId);
    maybeBoost(now);
}

void MlfqPolicy::maybeBoost(Time now) {
    if (config_.boostInterval <= 0) return;
    if (now - lastBoost_ < config_.boostInterval) return;
    lastBoost_ = now;
    std::deque<int> everyone;
    for (auto& q : queues_) {
        for (int id : q) everyone.push_back(id);
        q.clear();
    }
    for (auto& [id, lvl] : level_) {
        lvl = 0;
        allotmentUsed_[id] = 0;
    }
    for (int id : everyone) queues_[0].push_back(id);
}

DispatchDecision MlfqPolicy::dispatch(Time now) {
    maybeBoost(now);
    for (std::size_t lvl = 0; lvl < queues_.size(); ++lvl) {
        if (queues_[lvl].empty()) continue;
        int id = queues_[lvl].front();
        queues_[lvl].pop_front();
        Time slice = config_.quanta[lvl];
        if (lvl + 1 < queues_.size()) {
            Time remainingAllotment = config_.allotments[lvl] - allotmentUsed_[id];
            if (remainingAllotment > 0 && remainingAllotment < slice) slice = remainingAllotment;
        }
        return {id, slice};
    }
    return {};
}

bool MlfqPolicy::shouldPreemptCurrent(int currentTaskId, Time) const {
    std::size_t curLvl = level_.at(currentTaskId);
    for (std::size_t lvl = 0; lvl < curLvl; ++lvl) {
        if (!queues_[lvl].empty()) return true;
    }
    return false;
}

// ---------------------------------------------------------------- CfsLite

namespace {
constexpr int kNiceToWeight[40] = {
    88761, 71755, 56483, 46273, 36291, 29154, 23254, 18705, 14949, 11916,
    9548,  7620,  6100,  4904,  3906,  3121,  2501,  1991,  1586,  1277,
    1024,  820,   655,   526,   423,   335,   272,   215,   172,   137,
    110,   87,    70,    56,    45,    36,    29,    23,    18,    15,
};
}

int CfsLitePolicy::weightForNice(int nice) {
    int clamped = std::clamp(nice, -20, 19);
    return kNiceToWeight[clamped + 20];
}

CfsLitePolicy::CfsLitePolicy(Time granularity) : granularity_(granularity) {}

void CfsLitePolicy::registerTask(int taskId, int nice) {
    weight_[taskId] = weightForNice(nice);
    vruntime_[taskId] = 0.0;
}

void CfsLitePolicy::onArrive(int taskId, Time) {
    vruntime_[taskId] = std::max(vruntime_[taskId], minVruntimeSeen_);
    ready_.push_back(taskId);
}

void CfsLitePolicy::onReady(int taskId, Time) {
    vruntime_[taskId] = std::max(vruntime_[taskId], minVruntimeSeen_);
    ready_.push_back(taskId);
}

void CfsLitePolicy::onBlock(int taskId, Time, Time elapsed) {
    vruntime_[taskId] += static_cast<double>(elapsed) * 1024.0 / static_cast<double>(weight_[taskId]);
}

void CfsLitePolicy::onComplete(int taskId, Time, Time elapsed) {
    vruntime_[taskId] += static_cast<double>(elapsed) * 1024.0 / static_cast<double>(weight_[taskId]);
}

void CfsLitePolicy::onPreempted(int taskId, Time, Time elapsed) {
    vruntime_[taskId] += static_cast<double>(elapsed) * 1024.0 / static_cast<double>(weight_[taskId]);
    ready_.push_back(taskId);
}

DispatchDecision CfsLitePolicy::dispatch(Time) {
    if (ready_.empty()) return {};
    auto it = std::min_element(ready_.begin(), ready_.end(),
                                [&](int a, int b) { return vruntime_[a] < vruntime_[b]; });
    int id = *it;
    minVruntimeSeen_ = vruntime_[id];
    ready_.erase(it);
    return {id, granularity_};
}

bool CfsLitePolicy::shouldPreemptCurrent(int, Time) const { return false; }

}  // namespace schedsim
