#pragma once

// Process-wide unhandled-exception handler: on a fatal crash it writes a
// CrashReport (+ a minidump) into the crashes directory so the next launch can
// surface it. Install once at startup. Runtime/compile-verified — a real crash
// can't be exercised in CI.

#include <filesystem>
#include <string>

namespace veyra::service {

class CrashHandler {
public:
    static void install(std::string process, std::string version, std::string gitCommit,
                        std::filesystem::path crashesDir);
};

} // namespace veyra::service
