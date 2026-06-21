#pragma once

// AutoEq headphone-correction support: parse the AutoEq project's
// "ParametricEQ.txt" format (Preamp + a list of biquad filters) into our
// ParametricBand model, and load the vendored bundle (resources/autoeq/*.txt).
// Picking a headphone loads its correction straight into the Parametric EQ.

#include "veyra/Config.h" // ParametricBand

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace veyra {

struct AutoEqProfile {
    std::string                 name;        // headphone model (filename stem)
    float                       preampDb = 0.0f; // AutoEq pre-gain (usually negative)
    std::vector<ParametricBand> bands;
};

// Parse one AutoEq ParametricEQ.txt body. nullopt if it has no enabled filters.
std::optional<AutoEqProfile> parseAutoEq(const std::string& name, const std::string& text);

// Load every *.txt in dir (stem = model name), sorted by name. Best-effort.
std::vector<AutoEqProfile> loadAutoEqProfiles(const std::filesystem::path& dir);

} // namespace veyra
