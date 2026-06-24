#pragma once

// A second-order IIR section plus RBJ "Audio EQ Cookbook" coefficient designers.
// This is the building block for the graphic EQ, the bass/treble shelves, and
// the analyzer's anti-alias needs. Processing is Direct Form I (robust to
// coefficient modulation) and allocation-free.

#include <cmath>

namespace veyra::dsp {

struct BiquadCoeffs {
    // Normalised so a0 == 1: y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

    static BiquadCoeffs passthrough() noexcept { return {}; }
};

namespace detail {
inline BiquadCoeffs normalise(double b0, double b1, double b2,
                              double a0, double a1, double a2) noexcept
{
    const double inv = 1.0 / a0;
    return {static_cast<float>(b0 * inv), static_cast<float>(b1 * inv),
            static_cast<float>(b2 * inv), static_cast<float>(a1 * inv),
            static_cast<float>(a2 * inv)};
}
constexpr double kPi = 3.14159265358979323846;
} // namespace detail

// --- Coefficient designers (RBJ cookbook) ----------------------------------

inline BiquadCoeffs makePeaking(double fs, double freq, double q, double gainDb) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);

    return detail::normalise(1.0 + alpha * A, -2.0 * cw, 1.0 - alpha * A,
                             1.0 + alpha / A, -2.0 * cw, 1.0 - alpha / A);
}

inline BiquadCoeffs makeLowShelf(double fs, double freq, double q, double gainDb) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double twoSqrtAalpha = 2.0 * std::sqrt(A) * alpha;

    return detail::normalise(
        A * ((A + 1.0) - (A - 1.0) * cw + twoSqrtAalpha),
        2.0 * A * ((A - 1.0) - (A + 1.0) * cw),
        A * ((A + 1.0) - (A - 1.0) * cw - twoSqrtAalpha),
        (A + 1.0) + (A - 1.0) * cw + twoSqrtAalpha,
        -2.0 * ((A - 1.0) + (A + 1.0) * cw),
        (A + 1.0) + (A - 1.0) * cw - twoSqrtAalpha);
}

inline BiquadCoeffs makeHighShelf(double fs, double freq, double q, double gainDb) noexcept
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double twoSqrtAalpha = 2.0 * std::sqrt(A) * alpha;

    return detail::normalise(
        A * ((A + 1.0) + (A - 1.0) * cw + twoSqrtAalpha),
        -2.0 * A * ((A - 1.0) + (A + 1.0) * cw),
        A * ((A + 1.0) + (A - 1.0) * cw - twoSqrtAalpha),
        (A + 1.0) - (A - 1.0) * cw + twoSqrtAalpha,
        2.0 * ((A - 1.0) - (A + 1.0) * cw),
        (A + 1.0) - (A - 1.0) * cw - twoSqrtAalpha);
}

inline BiquadCoeffs makeLowPass(double fs, double freq, double q) noexcept
{
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double b1 = 1.0 - cw;
    return detail::normalise(b1 * 0.5, b1, b1 * 0.5,
                             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

inline BiquadCoeffs makeHighPass(double fs, double freq, double q) noexcept
{
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    const double b1 = -(1.0 + cw);
    return detail::normalise((1.0 + cw) * 0.5, b1, (1.0 + cw) * 0.5,
                             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

inline BiquadCoeffs makeNotch(double fs, double freq, double q) noexcept
{
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    return detail::normalise(1.0, -2.0 * cw, 1.0,
                             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

inline BiquadCoeffs makeBandPass(double fs, double freq, double q) noexcept
{
    // Constant 0 dB peak gain (RBJ "BPF, peak gain = Q").
    const double w0 = 2.0 * detail::kPi * freq / fs;
    const double cw = std::cos(w0), sw = std::sin(w0);
    const double alpha = sw / (2.0 * q);
    return detail::normalise(alpha, 0.0, -alpha,
                             1.0 + alpha, -2.0 * cw, 1.0 - alpha);
}

// --- The filter section -----------------------------------------------------

class Biquad {
public:
    void setCoeffs(const BiquadCoeffs& c) noexcept { c_ = c; }
    const BiquadCoeffs& coeffs() const noexcept { return c_; }

    void reset() noexcept { x1_ = x2_ = y1_ = y2_ = 0.0f; }

    float process(float x) noexcept
    {
        const float y = c_.b0 * x + c_.b1 * x1_ + c_.b2 * x2_
                        - c_.a1 * y1_ - c_.a2 * y2_;
        x2_ = x1_; x1_ = x;
        y2_ = y1_; y1_ = y + 1.0e-25f; // keep state away from subnormal range (-500 dBFS)
        return y;
    }

private:
    BiquadCoeffs c_;
    float x1_ = 0.0f, x2_ = 0.0f, y1_ = 0.0f, y2_ = 0.0f;
};

} // namespace veyra::dsp
