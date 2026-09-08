#include "schedsim/workload_io.hpp"

#include <nlohmann/json.hpp>

namespace schedsim {

using nlohmann::json;

WorkloadParseResult parseWorkloadJson(const std::string& source) {
    WorkloadParseResult result;
    json root;
    try {
        root = json::parse(source);
    } catch (const json::parse_error& e) {
        result.error = std::string("JSON syntax error: ") + e.what();
        return result;
    }

    if (!root.contains("tasks") || !root["tasks"].is_array()) {
        result.error = "workload must contain a \"tasks\" array";
        return result;
    }
    result.workload.name = root.value("name", std::string("workload"));

    for (const json& t : root["tasks"]) {
        if (!t.contains("id") || !t.contains("bursts") || !t["bursts"].is_array()) {
            result.error = "every task needs an \"id\" and a \"bursts\" array";
            return result;
        }
        Task task;
        task.id = t["id"].get<int>();
        task.name = t.value("name", "task" + std::to_string(task.id));
        task.arrival = t.value("arrival", 0);
        task.priority = t.value("priority", 0);
        task.nice = t.value("nice", 0);

        for (const json& b : t["bursts"]) {
            if (b.contains("cpu")) {
                task.bursts.push_back({BurstKind::Cpu, b["cpu"].get<Time>()});
            } else if (b.contains("io")) {
                task.bursts.push_back({BurstKind::Io, b["io"].get<Time>()});
            } else {
                result.error = "task " + std::to_string(task.id) + " has a burst with neither \"cpu\" nor \"io\"";
                return result;
            }
        }
        if (task.bursts.empty()) {
            result.error = "task " + std::to_string(task.id) + " has no bursts";
            return result;
        }
        result.workload.tasks.push_back(std::move(task));
    }

    if (result.workload.tasks.empty()) {
        result.error = "workload has no tasks";
        return result;
    }

    result.ok = true;
    return result;
}

std::string toWorkloadJson(const Workload& workload) {
    json root;
    root["name"] = workload.name;
    root["tasks"] = json::array();
    for (const Task& t : workload.tasks) {
        json bursts = json::array();
        for (const Burst& b : t.bursts) {
            bursts.push_back(b.kind == BurstKind::Cpu ? json{{"cpu", b.duration}} : json{{"io", b.duration}});
        }
        root["tasks"].push_back({
            {"id", t.id},
            {"name", t.name},
            {"arrival", t.arrival},
            {"priority", t.priority},
            {"nice", t.nice},
            {"bursts", bursts},
        });
    }
    return root.dump(2);
}

}  // namespace schedsim
