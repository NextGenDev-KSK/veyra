#pragma once

#include "veyra/Config.h"
#include "veyra/ipc/SharedMemory.h"
#include "veyra/ipc/SharedParameters.h"

namespace veyra {
class Logger;
}

namespace veyra::service {

// Owns the microphone shared parameter block and publishes the config's voice
// settings to the capture APO (single writer), mirroring ApoPublisher.
class MicPublisher {
public:
    explicit MicPublisher(Logger* log = nullptr) : log_(log) {}

    bool start();
    void publish(const Config& config);

private:
    Logger*                        log_;
    ipc::SharedMemoryRegion        region_;
    ipc::VeyraMicSharedParameters* params_ = nullptr;
};

} // namespace veyra::service
