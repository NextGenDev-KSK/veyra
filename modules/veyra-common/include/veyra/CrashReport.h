#pragma once

// Crash report model + on-disk store. The crash handler (service-side, runtime)
// fills one of these and writes it to %APPDATA%\Veyra\crashes; on the next
// launch the app reads the most recent one to show a "previous session crashed"
// banner. JSON is kept out of this header.

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace veyra {

struct CrashReport {
    std::string version;       // app version at crash time
    std::string gitCommit;     // build commit
    std::string timestamp;     // ISO-8601 UTC
    std::string process;       // "veyra-service" | "veyra" | ...
    uint32_t    exceptionCode = 0;
    uint64_t    address = 0;   // faulting instruction address
    std::string module;        // module the fault occurred in
    std::string message;       // human-readable summary

    std::string toJson() const;
    static std::optional<CrashReport> fromJson(const std::string& text);
};

// Write a timestamped crash-<...>.json into crashesDir. Returns the file path,
// or an empty path on failure. Safe to call from a crash handler.
std::filesystem::path writeCrashReport(const CrashReport& report,
                                       const std::filesystem::path& crashesDir);

// Most-recently-written crash report in crashesDir (for the next-launch banner),
// or nullopt if there are none / the dir is missing.
std::optional<CrashReport> latestCrashReport(const std::filesystem::path& crashesDir);

} // namespace veyra
