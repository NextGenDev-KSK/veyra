#pragma once

// ITD-aware HRIR interpolation. Naively crossfading two HRIRs measured at
// adjacent azimuths combs badly: they have different inter-aural delays, so the
// two direct-arrival peaks land at different samples and partially cancel —
// audible as smeared, jumpy localization. Instead we align the two IRs by their
// onset (the direct-arrival peak), crossfade the aligned shapes, and place the
// result at the *interpolated* delay. The magnitude blends smoothly and the ITD
// moves continuously, so panning between measurements is artifact-free.
//
// With t at an endpoint (or a == b) this reduces exactly to the source IR, so
// queries that land on a measured azimuth are unchanged.

#include <algorithm>
#include <cmath>
#include <vector>

namespace veyra::dsp {

// Index of the largest-magnitude sample (direct-arrival proxy for the ITD).
inline int hrirOnset(const std::vector<float>& x) noexcept
{
    int best = 0;
    float m = 0.0f;
    for (int i = 0; i < (int) x.size(); ++i)
    {
        const float a = std::fabs(x[i]);
        if (a > m) { m = a; best = i; }
    }
    return best;
}

inline void hrirInterpAligned(const std::vector<float>& a, const std::vector<float>& b,
                              float t, std::vector<float>& out)
{
    const int n = (int) std::min(a.size(), b.size());
    out.assign((size_t) n, 0.0f);
    if (n == 0)
        return;

    const int oa = hrirOnset(a);
    const int ob = hrirOnset(b);
    const int target = (int) std::lround((1.0f - t) * (float) oa + t * (float) ob);
    for (int i = 0; i < n; ++i)
    {
        const int ia = oa + i - target; // sample a relative to its own onset
        const int ib = ob + i - target;
        const float va = (ia >= 0 && ia < n) ? a[(size_t) ia] : 0.0f;
        const float vb = (ib >= 0 && ib < n) ? b[(size_t) ib] : 0.0f;
        out[(size_t) i] = (1.0f - t) * va + t * vb;
    }
}

} // namespace veyra::dsp
