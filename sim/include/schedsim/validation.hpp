#ifndef SCHEDSIM_VALIDATION_HPP
#define SCHEDSIM_VALIDATION_HPP

#include <string>
#include <vector>

namespace schedsim {

// One row of the historical-kernel-vs-simulator comparison. See README
// "Design decisions" / "Historical validation" for the three-tier
// methodology this implements (direct / qualitative / not-comparable),
// and why absolute values are never compared -- only directions.
struct ComparisonRow {
    std::string metric;
    std::string kernelDirection;      // "rr_better" | "fair_better" | "tie"
    std::string simulationDirection;  // same vocabulary, or "not_modeled"
    std::string status;               // "concordant" | "discordant" | "not_comparable"
    std::string note;                 // the actual real and simulated numbers, and why
};

// Runs the actual simulations this validation depends on and returns
// real, freshly-computed results every time -- never a hard-coded
// verdict. If the simulator disagrees with the historical kernel
// measurement on a qualitative-only row, that disagreement is reported
// as "discordant", not adjusted away. See README "Design decisions".
std::vector<ComparisonRow> runHistoricalValidation();

std::string renderValidationReport(const std::vector<ComparisonRow>& rows);

}  // namespace schedsim

#endif
