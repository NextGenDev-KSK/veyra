#pragma once

// Background update checker: fetches the GitHub releases feed over HTTPS
// (WinHTTP), runs the pure veyra::UpdateChecker against the current version, and
// caches the result. Check-only — downloading/applying an update is a separate,
// signature-verified step (not implemented). Runs on its own thread; re-checks
// daily while the service is up.

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "veyra/Updater.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

class UpdaterClient {
public:
    explicit UpdaterClient(Logger* log = nullptr) : log_(log) {}
    ~UpdaterClient();

    void start(std::string currentVersion, std::string feedUrl);
    void stop();

    UpdateInfo latest() const;

private:
    void run();

    Logger*            log_;
    std::string        current_, url_;
    std::thread        thread_;
    std::atomic<bool>  running_{false};
    mutable std::mutex mutex_;
    UpdateInfo         info_;
};

} // namespace veyra::service
