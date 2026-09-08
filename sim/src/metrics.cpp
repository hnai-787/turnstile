#include "schedsim/metrics.hpp"

#include <cmath>

namespace schedsim {

double jainIndex(const std::vector<double>& values) {
    if (values.empty()) return 1.0;
    double sum = 0.0, sumSquares = 0.0;
    for (double v : values) {
        sum += v;
        sumSquares += v * v;
    }
    if (sumSquares == 0.0) return 1.0;  // all zero -> treat as perfectly (trivially) fair
    double n = static_cast<double>(values.size());
    return (sum * sum) / (n * sumSquares);
}

}  // namespace schedsim
