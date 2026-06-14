#pragma once

// Low-level framed read/write helpers shared by the pipe server and client.
// Both ends speak the protocol in Protocol.h over a message-mode named pipe,
// so one ReadFile yields one logical message (looping only on ERROR_MORE_DATA).

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "veyra/ipc/Protocol.h"

namespace veyra::ipc {

// Read one whole message from a (blocking) message-mode pipe handle.
// Returns false on disconnect, error, or a malformed frame.
bool readMessage(HANDLE pipe, Message& out);

// Write one whole message in a single WriteFile. Returns false on error or a
// short write.
bool writeMessage(HANDLE pipe, const Message& msg);

} // namespace veyra::ipc
