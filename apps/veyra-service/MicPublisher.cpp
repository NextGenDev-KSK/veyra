#include "MicPublisher.h"

#include "veyra/Logging.h"

namespace veyra::service {

bool MicPublisher::start()
{
    if (!region_.create(ipc::kSharedMicParametersName, sizeof(ipc::VeyraMicSharedParameters)))
    {
        if (log_)
            log_->error("MicPublisher: failed to create mic parameter block");
        return false;
    }

    params_ = static_cast<ipc::VeyraMicSharedParameters*>(region_.data());

    if (region_.createdNew())
        ipc::publishMicParameters(params_, ipc::VeyraMicParamsPayload{});

    if (log_)
        log_->info("MicPublisher: mic parameter block ready");
    return true;
}

void MicPublisher::publish(const Config& config)
{
    if (!params_)
        return;

    const auto& v = config.voice;

    ipc::VeyraMicParamsPayload p;
    p.enabled           = v.enabled ? 1u : 0u;
    p.highPassHz        = v.highPassHz;
    p.noiseSuppression  = v.noiseSuppression;
    p.compressionAmount = v.compressionAmount;
    p.deEssAmount       = v.deEssAmount;
    p.presenceDb        = v.presenceDb;
    p.outputGainDb      = v.outputGainDb;
    p.sideToneLevel     = v.sideToneLevel;

    ipc::publishMicParameters(params_, p);
}

} // namespace veyra::service
