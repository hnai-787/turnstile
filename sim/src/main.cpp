#include <fstream>
#include <iostream>
#include <sstream>

#include "schedsim/report.hpp"
#include "schedsim/runner.hpp"
#include "schedsim/validation.hpp"
#include "schedsim/workload_io.hpp"
#include "schedsim/workload_presets.hpp"

using namespace schedsim;

namespace {

void printHelp() {
    std::cout <<
        "schedsim -- CPU scheduling algorithm simulation and benchmarking framework\n\n"
        "USAGE:\n"
        "  schedsim run <workload.json|preset> --algo rr|priority|mlfq|cfs-lite [--quantum N] [--format text|json]\n"
        "  schedsim compare <workload.json|preset> [--algos rr,priority,mlfq,cfs-lite]\n"
        "  schedsim validate\n"
        "  schedsim presets\n"
        "  schedsim export <preset> -o <file.json>\n"
        "  schedsim --help\n\n"
        "Presets: cpu-bound-equal, cpu-bound-mixed, interactive-mix, nice-mix, starvation\n";
}

std::optional<Workload> loadPresetByName(const std::string& name) {
    if (name == "cpu-bound-equal") return presets::cpuBoundEqual();
    if (name == "cpu-bound-mixed") return presets::cpuBoundMixed();
    if (name == "interactive-mix") return presets::interactiveMix();
    if (name == "nice-mix") return presets::niceMix();
    if (name == "starvation") return presets::starvation();
    return std::nullopt;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool loadWorkload(const std::string& spec, Workload& out) {
    if (auto preset = loadPresetByName(spec)) {
        out = *preset;
        return true;
    }
    if (endsWith(spec, ".json")) {
        WorkloadParseResult r = parseWorkloadJson(readFile(spec));
        if (!r.ok) {
            std::cerr << "Error: " << r.error << "\n";
            return false;
        }
        out = r.workload;
        return true;
    }
    std::cerr << "Error: \"" << spec << "\" is neither a known preset nor a .json workload file\n";
    return false;
}

std::vector<std::string> splitCommas(const std::string& s) {
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) result.push_back(tok);
    return result;
}

int cmdRun(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Error: run requires a workload\n";
        return 2;
    }
    Workload workload;
    if (!loadWorkload(argv[0], workload)) return 2;

    AlgoSpec spec;
    spec.name = "rr";
    std::string format = "text";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--algo" && i + 1 < argc) spec.name = argv[++i];
        else if (arg == "--quantum" && i + 1 < argc) spec.rrQuantum = std::stoll(argv[++i]);
        else if (arg == "--format" && i + 1 < argc) format = argv[++i];
        else { std::cerr << "Error: unrecognized option \"" << arg << "\"\n"; return 2; }
    }

    RunResult result = runWorkload(workload, spec);
    std::cout << (format == "json" ? renderRunResultJson(result) : renderRunResult(result)) << "\n";
    return 0;
}

int cmdCompare(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Error: compare requires a workload\n";
        return 2;
    }
    Workload workload;
    if (!loadWorkload(argv[0], workload)) return 2;

    std::vector<std::string> algos = {"rr", "priority", "mlfq", "cfs-lite"};
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--algos" && i + 1 < argc) algos = splitCommas(argv[++i]);
    }

    std::vector<RunResult> results;
    for (const std::string& algo : algos) {
        AlgoSpec spec;
        spec.name = algo;
        results.push_back(runWorkload(workload, spec));
    }
    std::cout << "Workload: " << workload.name << " (" << workload.tasks.size() << " tasks)\n\n";
    std::cout << renderComparison(results) << "\n";
    return 0;
}

int cmdValidate() {
    auto rows = runHistoricalValidation();
    std::cout << renderValidationReport(rows);
    return 0;
}

int cmdPresets() {
    std::cout << "Available presets:\n";
    for (const char* p : {"cpu-bound-equal", "cpu-bound-mixed", "interactive-mix", "nice-mix", "starvation"}) {
        std::cout << "  " << p << "\n";
    }
    return 0;
}

int cmdExport(int argc, char* argv[]) {
    if (argc < 1) {
        std::cerr << "Error: export requires a preset name\n";
        return 2;
    }
    auto preset = loadPresetByName(argv[0]);
    if (!preset) {
        std::cerr << "Error: unknown preset \"" << argv[0] << "\"\n";
        return 2;
    }
    std::string outPath;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) outPath = argv[++i];
    }
    std::string json = toWorkloadJson(*preset);
    if (outPath.empty()) {
        std::cout << json << "\n";
    } else {
        std::ofstream f(outPath);
        f << json;
        std::cout << "Wrote " << outPath << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        printHelp();
        return argc < 2 ? 2 : 0;
    }
    std::string command = argv[1];
    if (command == "run") return cmdRun(argc - 2, argv + 2);
    if (command == "compare") return cmdCompare(argc - 2, argv + 2);
    if (command == "validate") return cmdValidate();
    if (command == "presets") return cmdPresets();
    if (command == "export") return cmdExport(argc - 2, argv + 2);

    std::cerr << "Error: unrecognized command \"" << command << "\"\n\n";
    printHelp();
    return 2;
}
