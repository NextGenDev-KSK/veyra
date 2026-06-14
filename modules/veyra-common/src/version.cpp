#include "veyra/version.h"

// Phase 0 placeholder TU. Gives veyra-common a non-empty object file so the
// static library links cleanly across toolchains, and proves the generated
// version header is on the include path. Real shared utilities (config I/O,
// IPC framing, logging glue) land in later phases.

namespace veyra {

const char* versionString() noexcept
{
    return kVersionString;
}

} // namespace veyra
