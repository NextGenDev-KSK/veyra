#pragma once

namespace veyra::service {

// Hands control to the SCM dispatcher (the default launch path when Windows
// starts the service). Returns 0 on clean shutdown, otherwise a Win32 error.
int runAsService();

} // namespace veyra::service
