#ifndef SCHEDSIM_WORKLOAD_IO_HPP
#define SCHEDSIM_WORKLOAD_IO_HPP

#include <string>

#include "schedsim/types.hpp"

namespace schedsim {

struct WorkloadParseResult {
    bool ok = false;
    Workload workload;
    std::string error;
};

// {
//   "name": "...",
//   "tasks": [
//     {"id": 1, "name": "A", "arrival": 0, "priority": 0, "nice": 0,
//      "bursts": [{"cpu": 5}, {"io": 10}, {"cpu": 3}]}
//   ]
// }
WorkloadParseResult parseWorkloadJson(const std::string& source);
std::string toWorkloadJson(const Workload& workload);

}  // namespace schedsim

#endif
