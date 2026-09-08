#ifndef SCHEDSIM_REPORT_HPP
#define SCHEDSIM_REPORT_HPP

#include <string>
#include <vector>

#include "schedsim/metrics.hpp"

namespace schedsim {

std::string renderRunResult(const RunResult& result);
std::string renderRunResultJson(const RunResult& result);

// A side-by-side comparison table of system-level metrics across several
// algorithm runs on the *same* workload.
std::string renderComparison(const std::vector<RunResult>& results);

double averageMaxDispatchDelay(const RunResult& result);
double averageResponseTime(const RunResult& result);
double averageTurnaround(const RunResult& result);

}  // namespace schedsim

#endif
