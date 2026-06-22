#pragma once

// MIT KEMAR HRTF database loader. Indexes the "diffuse" compact set
// (third_party/hrtf/mit_kemar/diffuse/elev<E>/H<E>e<AZ>a.wav): each file is a
// stereo IR — channel 0 = left ear, channel 1 = right ear — for a source in the
// RIGHT hemisphere (azimuth 0..180). The left hemisphere is the mirror image
// (channels swapped), per KEMAR symmetry.
//
// getHrir() returns the (left, right) impulse responses for an arbitrary
// azimuth/elevation: nearest elevation, linear interpolation between the two
// bracketing measured azimuths, resampled to the engine rate, with caching.
// No JUCE; std::filesystem only. Loading is done off the audio thread (prepare).

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "spatial/HrirInterp.h"
#include "spatial/WavReader.h"

namespace veyra::dsp {

class HrtfDatabase {
public:
    // Index the diffuse compact set rooted at `diffuseDir`. Returns false if the
    // directory has no usable measurements (caller falls back to synthetic).
    bool load(const std::filesystem::path& diffuseDir)
    {
        index_.clear();
        cache_.clear();
        azByElev_.clear();
        loaded_ = false;

        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::is_directory(diffuseDir, ec))
            return false;

        for (const auto& elevEntry : fs::directory_iterator(diffuseDir, ec))
        {
            if (!elevEntry.is_directory())
                continue;
            const std::string name = elevEntry.path().filename().string(); // "elev0", "elev-10"
            if (name.rfind("elev", 0) != 0)
                continue;
            const int elev = std::atoi(name.c_str() + 4);
            for (const auto& f : fs::directory_iterator(elevEntry.path(), ec))
            {
                if (f.path().extension() != ".wav")
                    continue;
                int az = 0;
                if (!parseAzimuth(f.path().filename().string(), az))
                    continue;
                index_[{elev, az}] = f.path();
                azByElev_[elev].push_back(az);
            }
        }
        for (auto& [elev, list] : azByElev_)
        {
            std::sort(list.begin(), list.end());
            list.erase(std::unique(list.begin(), list.end()), list.end());
        }
        loaded_ = !index_.empty();
        return loaded_;
    }

    bool loaded() const noexcept { return loaded_; }
    int  measurementCount() const noexcept { return (int) index_.size(); }

    // azimuthDeg: 0 = front, negative = left, positive = right, wrapped to ±180.
    // Fills left/right with the resampled IR. Returns false if not loaded.
    bool getHrir(float azimuthDeg, float elevationDeg, double targetSampleRate,
                 std::vector<float>& left, std::vector<float>& right)
    {
        if (!loaded_)
            return false;

        const int elev = nearestElev(elevationDeg);
        const float az = wrap180(azimuthDeg);
        const bool leftHemisphere = az < 0.0f;
        const float mag = std::min(std::fabs(az), 180.0f);

        int aLo = 0, aHi = 0;
        float t = 0.0f;
        bracketAzimuth(elev, mag, aLo, aHi, t);

        const HrirPair& lo = fetch(elev, aLo);
        const HrirPair& hi = fetch(elev, aHi);
        if (lo.left.empty())
            return false;

        // Blend the bracketing measurements; swap ears for the left hemisphere.
        const auto& loL = leftHemisphere ? lo.right : lo.left;
        const auto& loR = leftHemisphere ? lo.left  : lo.right;
        const auto& hiL = leftHemisphere ? hi.right : hi.left;
        const auto& hiR = leftHemisphere ? hi.left  : hi.right;

        std::vector<float> mixL, mixR;
        hrirInterpAligned(loL, hiL, t, mixL); // ITD-aware (onset-aligned) blend
        hrirInterpAligned(loR, hiR, t, mixR);

        resample(mixL, lo.sampleRate, targetSampleRate, left);
        resample(mixR, lo.sampleRate, targetSampleRate, right);
        return !left.empty();
    }

private:
    struct HrirPair { std::vector<float> left, right; int sampleRate = 44100; };

    static bool parseAzimuth(const std::string& fn, int& az)
    {
        const auto e = fn.find('e');
        if (e == std::string::npos) return false;
        std::string d;
        for (size_t i = e + 1; i < fn.size() && std::isdigit((unsigned char) fn[i]); ++i)
            d += fn[i];
        if (d.empty()) return false;
        az = std::atoi(d.c_str());
        return true;
    }

    static float wrap180(float deg)
    {
        while (deg > 180.0f) deg -= 360.0f;
        while (deg < -180.0f) deg += 360.0f;
        return deg;
    }

    int nearestElev(float elevDeg) const
    {
        int best = 0, bestD = 1 << 30;
        for (const auto& [elev, list] : azByElev_)
        {
            const int d = std::abs((int) std::lround(elevDeg) - elev);
            if (d < bestD) { bestD = d; best = elev; }
        }
        return best;
    }

    void bracketAzimuth(int elev, float mag, int& lo, int& hi, float& t) const
    {
        const auto it = azByElev_.find(elev);
        if (it == azByElev_.end() || it->second.empty()) { lo = hi = 0; t = 0.0f; return; }
        const auto& a = it->second;
        if (mag <= (float) a.front()) { lo = hi = a.front(); t = 0.0f; return; }
        if (mag >= (float) a.back())  { lo = hi = a.back();  t = 0.0f; return; }
        for (size_t i = 1; i < a.size(); ++i)
            if ((float) a[i] >= mag)
            {
                lo = a[i - 1]; hi = a[i];
                const float span = (float) (hi - lo);
                t = span > 0.0f ? (mag - (float) lo) / span : 0.0f;
                return;
            }
        lo = hi = a.back(); t = 0.0f;
    }

    const HrirPair& fetch(int elev, int az)
    {
        const auto key = std::make_pair(elev, az);
        const auto c = cache_.find(key);
        if (c != cache_.end())
            return c->second;
        HrirPair p;
        const auto fi = index_.find(key);
        if (fi != index_.end())
        {
            WavData w;
            if (readWav(fi->second, w) && w.channels >= 2)
            {
                p.left = w.ch[0];
                p.right = w.ch[1];
                p.sampleRate = w.sampleRate;
            }
        }
        return cache_.emplace(key, std::move(p)).first->second;
    }

    static void resample(const std::vector<float>& in, int srcSr, double dstSr,
                         std::vector<float>& out)
    {
        if (in.empty() || srcSr <= 0) { out.clear(); return; }
        if (std::abs((double) srcSr - dstSr) < 1.0) { out = in; return; }
        const double ratio = dstSr / (double) srcSr;
        const size_t n = (size_t) std::max<double>(1.0, std::lround((double) in.size() * ratio));
        out.resize(n);
        for (size_t j = 0; j < n; ++j)
        {
            const double pos = (double) j / ratio;
            const size_t i0 = (size_t) pos;
            const float frac = (float) (pos - (double) i0);
            const float s0 = in[std::min(i0, in.size() - 1)];
            const float s1 = in[std::min(i0 + 1, in.size() - 1)];
            out[j] = s0 + frac * (s1 - s0);
        }
    }

    bool loaded_ = false;
    std::map<std::pair<int, int>, std::filesystem::path> index_; // (elev,az) -> file
    std::map<std::pair<int, int>, HrirPair> cache_;              // (elev,az) -> loaded IR
    std::map<int, std::vector<int>> azByElev_;                   // elev -> sorted azimuths
};

} // namespace veyra::dsp
