#pragma once

// Lightweight localization: a compiled-in English base catalog plus an optional
// per-language overlay loaded from resources/lang/<code>.json. Lookups resolve
// overlay -> English base -> the key itself (so a missing translation degrades
// to English, and a missing key surfaces visibly rather than crashing). JSON is
// kept out of this header so consumers don't depend on nlohmann.

#include <filesystem>
#include <string>
#include <unordered_map>

namespace veyra {

class Localization {
public:
    Localization(); // installs the English base catalog

    // Overlay a JSON object of { "key": "translation", ... }.
    void setOverlayJson(const std::string& json);

    // Load resources/lang/<code>.json as the overlay. "en" clears the overlay
    // (English base only). Returns false if a non-en file is missing/invalid.
    bool setLanguage(const std::string& code, const std::filesystem::path& langDir);

    void clearOverlay();

    std::string tr(const std::string& key) const;
    const std::string& language() const noexcept { return lang_; }

private:
    std::unordered_map<std::string, std::string> base_;
    std::unordered_map<std::string, std::string> overlay_;
    std::string lang_ = "en";
};

// Process-wide instance + convenience free function.
Localization& loc();
inline std::string tr(const std::string& key) { return loc().tr(key); }

} // namespace veyra
