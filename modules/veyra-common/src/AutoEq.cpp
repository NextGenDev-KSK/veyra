#include "veyra/AutoEq.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace veyra {

namespace {
// AutoEq filter type token -> ParametricBand::type
// (0 bell, 1 low-shelf, 2 high-shelf, 3 notch, 4 high-pass, 5 low-pass).
int typeFromToken(const std::string& t)
{
    if (t == "LSC" || t == "LS") return 1;
    if (t == "HSC" || t == "HS") return 2;
    if (t == "HP"  || t == "HPQ") return 4;
    if (t == "LP"  || t == "LPQ") return 5;
    return 0; // PK / default = bell
}
} // namespace

std::optional<AutoEqProfile> parseAutoEq(const std::string& name, const std::string& text)
{
    AutoEqProfile p;
    p.name = name;

    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream ls(line);
        std::string w;
        ls >> w;
        if (w == "Preamp:")
        {
            float v = 0.0f;
            if (ls >> v) p.preampDb = v;
            continue;
        }
        if (w == "Filter")
        {
            // Filter <n>: <ON|OFF> <TYPE> Fc <f> Hz Gain <g> dB Q <q>
            std::string idx, onoff, type, tFc, tHz, tGain, tDb, tQ;
            float f = 0.0f, g = 0.0f, q = 1.0f;
            ls >> idx >> onoff >> type >> tFc >> f >> tHz >> tGain >> g >> tDb >> tQ >> q;
            if (onoff != "ON" || f <= 0.0f)
                continue;
            ParametricBand b;
            b.enabled = true;
            b.type    = typeFromToken(type);
            b.freq    = f;
            b.gainDb  = g;
            b.q       = q > 0.0f ? q : 1.0f;
            p.bands.push_back(b);
            if (p.bands.size() >= 16) // matches ParametricEq::kMaxBands
                break;
        }
    }

    if (p.bands.empty())
        return std::nullopt;
    return p;
}

std::vector<AutoEqProfile> loadAutoEqProfiles(const std::filesystem::path& dir)
{
    std::vector<AutoEqProfile> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
        return out;

    for (const auto& e : std::filesystem::directory_iterator(dir, ec))
    {
        if (e.path().extension() != ".txt")
            continue;
        std::ifstream f(e.path(), std::ios::binary);
        if (!f)
            continue;
        std::ostringstream ss;
        ss << f.rdbuf();
        if (auto p = parseAutoEq(e.path().stem().string(), ss.str()))
            out.push_back(std::move(*p));
    }
    std::sort(out.begin(), out.end(),
              [](const AutoEqProfile& a, const AutoEqProfile& b) { return a.name < b.name; });
    return out;
}

} // namespace veyra
