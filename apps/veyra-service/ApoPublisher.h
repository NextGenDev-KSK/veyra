#pragma once

#include "veyra/Config.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/SharedParameters.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

// Owns the shared parameter block and publishes config-derived DSP parameters
// to the APO (single writer). Phase 2 maps the subset of Config that exists
// today; per-preset EQ etc. fill in as later phases extend Config.
class ApoPublisher {
public:
    explicit ApoPublisher(Logger* log = nullptr) : log_(log) {}

    bool start();
    void publish(const Config& config);

private:
    Logger*                          log_;
    ipc::SharedMemoryRegion          region_;
    ipc::VeyraSharedParameters*      params_ = nullptr;
};

} // namespace veyra::service
