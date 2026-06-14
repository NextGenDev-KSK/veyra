#include "ApoPublisher.h"

#include "veyra/Logging.h"

namespace veyra::service {

bool ApoPublisher::start()
{
    if (!region_.create(ipc::kSharedParametersName, sizeof(ipc::VeyraSharedParameters)))
    {
        if (log_)
            log_->error("ApoPublisher: failed to create shared parameter block");
        return false;
    }

    params_ = static_cast<ipc::VeyraSharedParameters*>(region_.data());

    // Freshly created mappings are zero-filled, which would mean a 0 generation
    // and a 0 volume gain. Publish sensible defaults so the APO starts correct.
    if (region_.createdNew())
        ipc::publishParameters(params_, ipc::VeyraParamsPayload{});

    if (log_)
        log_->info("ApoPublisher: shared parameter block ready");
    return true;
}

void ApoPublisher::publish(const Config& config)
{
    if (!params_)
        return;

    ipc::VeyraParamsPayload p; // in-struct defaults: flat EQ, width 1, gain 1
    p.bypass = config.masterEnabled ? 0u : 1u;
    p.volumeGain = static_cast<float>(config.masterVolumeGain);
    // Per-preset EQ / tone / dynamics map in here as Config grows (later phases).

    ipc::publishParameters(params_, p);
}

} // namespace veyra::service
