#include "schedsim/validation.hpp"

#include <sstream>

#include "schedsim/report.hpp"
#include "schedsim/runner.hpp"
#include "schedsim/workload_presets.hpp"

namespace schedsim {

namespace {

std::string fmt(double v, int precision = 3) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(precision);
    oss << v;
    return oss.str();
}

}  // namespace

std::vector<ComparisonRow> runHistoricalValidation() {
    std::vector<ComparisonRow> rows;

    // --- Row: CPU throughput -- NOT_COMPARABLE by construction ---
    rows.push_back({
        "CPU throughput (sysbench events/s; stress-ng bogo-ops/s)",
        "fair_better",
        "not_modeled",
        "not_comparable",
        "Real kernel: CFS/EEVDF 93.04 events/s vs RR 76.82 events/s (sysbench); ~tie on stress-ng. "
        "In a zero-overhead discrete-event simulator, every policy completes the same total CPU work in "
        "the same busy time by construction -- raw throughput cannot differ between scheduling policies "
        "here. The real difference reflects implementation/hardware effects (context-switch cost, cache "
        "behavior, accounting overhead) this simulator does not model. See README 'Design decisions'.",
    });

    // --- Row: file I/O throughput -- NOT_COMPARABLE ---
    rows.push_back({
        "File I/O throughput (sysbench fileio read)",
        "rr_better",
        "not_modeled",
        "not_comparable",
        "Real kernel: RR 15.37 MiB/s vs CFS/EEVDF 11.34 MiB/s. This simulator's I/O bursts are abstract "
        "blocking periods, not a modeled filesystem/storage stack, so this metric has no simulator "
        "counterpart at all.",
    });

    // --- Row: scheduling delay -- QUALITATIVE_ONLY ---
    {
        Workload w = presets::interactiveMix();
        RunResult rr = runWorkload(w, AlgoSpec{"rr", 100});
        RunResult cfs = runWorkload(w, AlgoSpec{"cfs-lite", 100, {}, MlfqConfig::ostepDemo(), 4});
        double rrDelay = averageMaxDispatchDelay(rr);
        double cfsDelay = averageMaxDispatchDelay(cfs);
        std::string simDir = rrDelay < cfsDelay ? "rr_better" : (cfsDelay < rrDelay ? "fair_better" : "tie");
        std::string status = (simDir == "rr_better") ? "concordant" : "discordant";
        rows.push_back({
            "Scheduling delay (perf sched latency total vs. simulated avg max-dispatch-delay, interactive-mix workload)",
            "rr_better",
            simDir,
            status,
            "Real kernel: RR total scheduling delay 896.388ms vs CFS/EEVDF 1119.708ms (lower is better). "
            "Simulated (interactive-mix, avg of each task's worst ready-to-dispatch gap): RR=" + fmt(rrDelay) +
            ", cfs-lite=" + fmt(cfsDelay) + ". Compared as DIRECTION ONLY -- the two measurements are not "
            "the same unit or methodology (see README).",
        });
    }

    // --- Row: CPU-share fairness -- QUALITATIVE_ONLY ---
    {
        Workload w = presets::cpuBoundEqual();
        RunResult rr = runWorkload(w, AlgoSpec{"rr", 100});
        RunResult cfs = runWorkload(w, AlgoSpec{"cfs-lite", 100, {}, MlfqConfig::ostepDemo(), 4});
        double rrJain = rr.system.jainFairnessRaw;
        double cfsJain = cfs.system.jainFairnessRaw;
        std::string simDir = rrJain > cfsJain ? "rr_better" : (cfsJain > rrJain ? "fair_better" : "tie");
        std::string status = (simDir == "rr_better" || simDir == "tie") ? "concordant" : "discordant";
        rows.push_back({
            "CPU-share fairness (top snapshot CPU% distribution vs. Jain fairness index, cpu-bound-equal workload)",
            "rr_better",
            simDir,
            status,
            "Real kernel: RR CPU% distribution across identical 'yes' processes was visibly more uniform; "
            "CFS/EEVDF's was skewed. Simulated (4 equal-weight CPU-bound tasks): RR Jain index=" + fmt(rrJain, 5) +
            ", cfs-lite Jain index=" + fmt(cfsJain, 5) + ". Both are expected to be near-1.0 in an idealized "
            "simulator with equal weights and no wakeup-preemption granularity effects -- the real kernel's "
            "skew likely comes from scheduling-granularity and migration effects this simulator does not model.",
        });
    }

    return rows;
}

std::string renderValidationReport(const std::vector<ComparisonRow>& rows) {
    std::ostringstream oss;
    oss << "Historical kernel validation (validation/kernel-6.12.25/)\n";
    oss << "===========================================================\n";
    oss << "Methodology: DIRECT / QUALITATIVE_ONLY (direction of effect only) / NOT_COMPARABLE.\n";
    oss << "See README 'Design decisions' for why absolute values are never compared.\n\n";
    for (const ComparisonRow& row : rows) {
        oss << "[" << row.status << "] " << row.metric << "\n";
        oss << "  kernel direction:     " << row.kernelDirection << "\n";
        oss << "  simulation direction: " << row.simulationDirection << "\n";
        oss << "  " << row.note << "\n\n";
    }
    return oss.str();
}

}  // namespace schedsim
