#include "veyra/version.h"

// Provides veyra::versionString() and the kVersionString / kGitCommit constants
// generated from CMakeLists.txt into version.h.in. The library also contains
// Config I/O, IPC framing, logging, Paths, and shared-memory plumbing.

namespace veyra {

const char* versionString() noexcept
{
    return kVersionString;
}

} // namespace veyra
