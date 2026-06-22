#include "ApoPublisher.h"

#include <algorithm>

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

    const auto& e = config.enhancement;

    ipc::VeyraParamsPayload p; // in-struct defaults: flat EQ, width 1, gain 1
    p.bypass = config.masterEnabled ? 0u : 1u;

    // Effective output gain = master trim × the Volume Gain knob, clamped to the
    // DSP's sane 0..300% range.
    const float master = static_cast<float>(config.masterVolumeGain);
    p.volumeGain = std::clamp(master * e.volumeGain, 0.0f, 3.0f);

    for (size_t i = 0; i < e.eqBandsDb.size(); ++i)
        p.eqBandsDb[i] = e.eqBandsDb[i];
    p.bassBoostDb       = e.bassBoostDb;
    p.trebleDb          = e.trebleDb;
    p.stereoWidth       = e.stereoWidth;
    p.compressionAmount = e.compressionAmount;
    p.exciterAmount     = e.exciterAmount;
    p.saturationAmount  = e.saturationAmount;
    p.saturationMode    = (uint32_t) (e.saturationMode < 0 ? 0 : e.saturationMode);
    p.multibandWidth    = e.multibandWidth;
    p.transientAmount   = e.transientAmount;
    p.bassEnhanceAmount = e.bassEnhanceAmount;
    p.crossfeedAmount     = config.spatial.enabled ? config.spatial.crossfeed : 0.0f;
    p.virtualizationAmount = config.spatial.enabled ? config.spatial.virtualization : 0.0f;
    p.nightModeAmount     = config.loudness.nightModeAmount;
    p.loudnessMatch       = config.loudness.loudnessMatch ? 1u : 0u;
    p.loudnessTargetLufs  = config.loudness.targetLufs;
    p.equalLoudness       = config.loudness.equalLoudness ? 1u : 0u;
    p.referenceMode       = config.referenceMode ? 1u : 0u;
    // Reverb has no DSP stage yet; e.reverbAmount is carried in Config for the UI.

    ipc::publishParameters(params_, p);
}

} // namespace veyra::service
