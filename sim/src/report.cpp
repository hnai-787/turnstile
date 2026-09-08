#include "schedsim/report.hpp"

#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

namespace schedsim {

using nlohmann::json;

double averageMaxDispatchDelay(const RunResult& result) {
    if (result.perTask.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& [id, m] : result.perTask) sum += static_cast<double>(m.maxDispatchDelay);
    return sum / static_cast<double>(result.perTask.size());
}

double averageResponseTime(const RunResult& result) {
    if (result.perTask.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& [id, m] : result.perTask) sum += static_cast<double>(m.response);
    return sum / static_cast<double>(result.perTask.size());
}

double averageTurnaround(const RunResult& result) {
    if (result.perTask.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& [id, m] : result.perTask) sum += static_cast<double>(m.turnaround);
    return sum / static_cast<double>(result.perTask.size());
}

std::string renderRunResult(const RunResult& result) {
    std::ostringstream oss;
    oss << "Policy: " << result.policyName << "\n";
    oss << std::left << std::setw(6) << "Task" << std::setw(10) << "Arrival" << std::setw(10) << "Complete"
        << std::setw(10) << "Turnrnd" << std::setw(10) << "Response" << std::setw(10) << "ReadyWt" << std::setw(10)
        << "CpuSvc" << std::setw(10) << "MaxDelay" << std::setw(8) << "Disp" << std::setw(8) << "Preempt" << "\n";
    for (const auto& [id, m] : result.perTask) {
        oss << std::left << std::setw(6) << id << std::setw(10) << m.arrival << std::setw(10) << m.completion
            << std::setw(10) << m.turnaround << std::setw(10) << m.response << std::setw(10) << m.readyWait
            << std::setw(10) << m.cpuService << std::setw(10) << m.maxDispatchDelay << std::setw(8)
            << m.dispatchCount << std::setw(8) << m.preemptionCount << "\n";
    }
    oss << "\nSystem: duration=" << result.system.simulationDuration << " utilization="
        << std::fixed << std::setprecision(3) << result.system.utilization << " throughput="
        << std::setprecision(5) << result.system.throughput << " contextSwitches=" << result.system.contextSwitches
        << " preemptions=" << result.system.preemptions << " jainFairnessRaw=" << std::setprecision(4)
        << result.system.jainFairnessRaw << "\n";
    return oss.str();
}

std::string renderRunResultJson(const RunResult& result) {
    json root;
    root["policy"] = result.policyName;
    root["tasks"] = json::array();
    for (const auto& [id, m] : result.perTask) {
        root["tasks"].push_back({
            {"id", id},
            {"arrival", m.arrival},
            {"completion", m.completion},
            {"turnaround", m.turnaround},
            {"response", m.response},
            {"ready_wait", m.readyWait},
            {"cpu_service", m.cpuService},
            {"io_blocked", m.ioBlocked},
            {"max_dispatch_delay", m.maxDispatchDelay},
            {"dispatch_count", m.dispatchCount},
            {"preemption_count", m.preemptionCount},
            {"slowdown", m.slowdown},
        });
    }
    root["system"] = {
        {"duration", result.system.simulationDuration},
        {"busy_time", result.system.busyTime},
        {"utilization", result.system.utilization},
        {"throughput", result.system.throughput},
        {"context_switches", result.system.contextSwitches},
        {"preemptions", result.system.preemptions},
        {"jain_fairness_raw", result.system.jainFairnessRaw},
    };
    return root.dump(2);
}

std::string renderComparison(const std::vector<RunResult>& results) {
    std::ostringstream oss;
    oss << std::left << std::setw(12) << "Policy" << std::setw(12) << "AvgResp" << std::setw(12) << "AvgTurn"
        << std::setw(12) << "AvgDelay" << std::setw(10) << "Jain" << std::setw(8) << "CtxSw" << std::setw(10)
        << "Preempt" << std::setw(12) << "Throughput" << "\n";
    for (const RunResult& r : results) {
        oss << std::left << std::setw(12) << r.policyName << std::setw(12) << std::fixed << std::setprecision(2)
            << averageResponseTime(r) << std::setw(12) << averageTurnaround(r) << std::setw(12)
            << averageMaxDispatchDelay(r) << std::setw(10) << std::setprecision(4) << r.system.jainFairnessRaw
            << std::setw(8) << r.system.contextSwitches << std::setw(10) << r.system.preemptions << std::setw(12)
            << std::setprecision(6) << r.system.throughput << "\n";
    }
    return oss.str();
}

}  // namespace schedsim
