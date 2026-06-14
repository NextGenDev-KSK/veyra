#pragma once

#include <cmath>
#include <vector>

namespace th {

constexpr double kPi = 3.14159265358979323846;

inline std::vector<float> sine(double freq, double fs, int n, float amp = 1.0f)
{
    std::vector<float> v(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        v[static_cast<size_t>(i)] =
            amp * static_cast<float>(std::sin(2.0 * kPi * freq * i / fs));
    return v;
}

// RMS over the trailing fraction of a buffer (skips filter/envelope transients).
inline double rmsTail(const std::vector<float>& v, double tailFraction = 0.5)
{
    const int n = static_cast<int>(v.size());
    const int start = static_cast<int>(n * (1.0 - tailFraction));
    double sum = 0.0;
    int count = 0;
    for (int i = start; i < n; ++i) { sum += double(v[i]) * v[i]; ++count; }
    return count > 0 ? std::sqrt(sum / count) : 0.0;
}

inline float maxAbs(const std::vector<float>& v)
{
    float m = 0.0f;
    for (float x : v) m = std::max(m, std::fabs(x));
    return m;
}

inline double linToDb(double lin) { return 20.0 * std::log10(std::max(lin, 1e-12)); }

} // namespace th
