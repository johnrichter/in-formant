#ifndef MAIN_CONTEXT_SUMMARY_H
#define MAIN_CONTEXT_SUMMARY_H

#include <algorithm>
#include <cmath>
#include <vector>

// The statistics of the values of a track in a time range. The code has no dependency on the data store,
// so a test can run it.

namespace Main {

    struct Summary {
        size_t frames = 0;
        std::vector<double> values;   // sorted

        bool empty() const { return values.empty(); }
        double mean() const {
            double sum = 0;
            for (double v : values) sum += v;
            return sum / (double) values.size();
        }
        // The sample standard deviation, with N - 1 in the divisor. One value has no spread.
        double sd() const {
            if (values.size() < 2) return 0.0;
            const double m = mean();
            double sum = 0;
            for (double v : values) sum += (v - m) * (v - m);
            return std::sqrt(sum / (double) (values.size() - 1));
        }

        // The value at a fraction of the sorted values, with a linear step between two values.
        double percentile(double fraction) const {
            const double position = fraction * (double) (values.size() - 1);
            const size_t low = (size_t) std::floor(position);
            const size_t high = std::min(low + 1, values.size() - 1);
            return values[low] + (position - (double) low) * (values[high] - values[low]);
        }
        double median() const { return percentile(0.5); }
        double share() const { return frames == 0 ? 0.0 : 100.0 * (double) values.size() / (double) frames; }
    };

}

#endif // MAIN_CONTEXT_SUMMARY_H
