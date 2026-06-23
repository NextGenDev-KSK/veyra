#pragma once

// Enumerates active render (output) endpoints in the user session for the
// Devices screen + AudioBridge routing pickers. Runs in the UI process (a normal
// user app), so it sees the user's devices directly — no service round-trip.

#include <string>
#include <vector>

#include "VeyraGui.h"

namespace veyra::ui {

struct OutputDevice {
    std::string  id;        // MMDevice endpoint id (matches config.bridge.*DeviceId)
    juce::String name;      // friendly name for display
    juce::String type;      // form-factor label (Headphones / Speakers / HDMI / Mic / …)
    bool         isDefault = false;
};

std::vector<OutputDevice> listRenderEndpoints();
std::vector<OutputDevice> listCaptureEndpoints();

// APO-first preferred output: make the given render endpoint the Windows default
// (all roles) via IPolicyConfig, so the system-wide APO processes it. Returns
// true on success; no-op + false on empty id. Caller runs on the COM-initialised
// message thread. [RUNTIME VERIFICATION REQUIRED — changes the system default.]
bool setDefaultRenderEndpoint(const std::string& endpointId);

} // namespace veyra::ui
